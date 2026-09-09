export type ComponentState = 'HEALTHY' | 'DEGRADED' | 'FAILED' | 'UNKNOWN';
export interface TwinEvent { time_s: number; type: string; message: string; level: 'info' | 'warn' | 'critical' }
export interface TwinSnapshot {
  time_s: number;
  aircraft: { x_m: number; y_m: number; z_m: number; altitude_m: number; pitch_rad: number; roll_rad: number; airspeed_ms: number };
  target: { x_m: number; y_m: number; altitude_m: number };
  actuators: { rotor_rpm: number; left_servo_deg: number; right_servo_deg: number };
  mission: string; health: string; safety_mode: string; active_fault: string | null;
  fc1: { status: string; components: Record<string, ComponentState> }; fc2: { status: string; components: Record<string, ComponentState> };
  hil: { loop_hz: number; deadline_misses: number }; events: TwinEvent[];
}
const healthy = { mcu: 'HEALTHY', transport: 'HEALTHY', sensors: 'HEALTHY', control: 'HEALTHY', actuators: 'HEALTHY', supervision: 'HEALTHY', safety: 'HEALTHY' } as Record<string, ComponentState>;
export function createDemoSnapshots(): TwinSnapshot[] {
  const events: TwinEvent[] = [
    { time_s: 0, type: 'SYSTEM', message: 'HIL session initialized', level: 'info' }, { time_s: 1.2, type: 'FC1', message: 'Flight controller online', level: 'info' }, { time_s: 1.4, type: 'FC2', message: 'Safety controller online', level: 'info' }, { time_s: 3, type: 'MISSION', message: 'Takeoff authorized', level: 'info' }, { time_s: 12, type: 'MISSION', message: 'STATION_KEEPING', level: 'info' }, { time_s: 15, type: 'FAULT', message: 'INVALID_SENSOR_DATA', level: 'warn' }, { time_s: 15.01, type: 'DETECTION', message: 'SENSOR_VALIDATION_FAILED', level: 'warn' }, { time_s: 15.02, type: 'SAFETY', message: 'COMPENSATED', level: 'warn' }, { time_s: 18, type: 'RECOVERY', message: 'SENSOR_VALID', level: 'info' }, { time_s: 18.01, type: 'HEALTH', message: 'HEALTHY · NORMAL', level: 'info' },
  ];
  return Array.from({ length: 181 }, (_, index) => {
    const time = index * 0.12; const inFault = time >= 15 && time < 18; const altitude = time < 12 ? 92 + time * 0.67 : 100 + Math.sin(time * 1.5) * 0.35;
    return { time_s: time, aircraft: { x_m: 4.1 + Math.sin(time * 0.36) * 1.1, y_m: -1.2 + Math.cos(time * 0.31) * 0.8, z_m: -altitude, altitude_m: altitude, pitch_rad: Math.sin(time * 0.7) * 0.035, roll_rad: Math.cos(time * 0.62) * 0.028, airspeed_ms: 17.5 + Math.sin(time) * 0.4 }, target: { x_m: 4, y_m: -1, altitude_m: 100 }, actuators: { rotor_rpm: 5350 + Math.sin(time * 2) * 80, left_servo_deg: 1.3 + Math.sin(time) * 0.4, right_servo_deg: -0.8 + Math.cos(time * 0.8) * 0.3 }, mission: time < 12 ? 'CLIMB' : 'STATION_KEEPING', health: inFault ? 'DEGRADED' : 'HEALTHY', safety_mode: inFault ? 'COMPENSATED' : 'NORMAL', active_fault: inFault ? 'INVALID_SENSOR_DATA' : null, fc1: { status: 'ONLINE', components: { ...healthy, sensors: inFault ? 'DEGRADED' : 'HEALTHY' } }, fc2: { status: 'ONLINE', components: { ...healthy } }, hil: { loop_hz: 200 + Math.sin(time) * 0.15, deadline_misses: 0 }, events: events.filter((event) => event.time_s <= time) };
  });
}
