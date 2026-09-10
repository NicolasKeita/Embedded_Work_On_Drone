'use client';

import { useEffect, useMemo, useRef, useState } from 'react';
import { Activity, AlertTriangle, Box, ChevronDown, CircleDot, Gauge, Pause, Play, Radio, RotateCcw, Upload } from 'lucide-react';
import { AircraftScene } from '@/components/aircraft-scene';
import { AvionicsScene } from '@/components/avionics-scene';
import { TelemetryCharts } from '@/components/telemetry-charts';
import { createDemoSnapshots, createIdleSnapshot, type TwinSnapshot } from '@/lib/twin-data';

const WS_URL = 'ws://localhost:8765/twin';

function StatusDot({ tone = 'green' }: { tone?: 'green' | 'amber' | 'red' | 'gray' }) { return <span className={`status-dot ${tone}`} />; }
function Metric({ label, value, unit }: { label: string; value: string; unit?: string }) { return <div className="metric"><span>{label}</span><strong>{value}{unit && <small>{unit}</small>}</strong></div>; }

export default function Home() {
  const demo = useMemo(() => createDemoSnapshots(), []);
  const idle = useMemo(() => createIdleSnapshot(), []);
  const [mode, setMode] = useState<'replay' | 'live'>('live');
  const [playing, setPlaying] = useState(true);
  const [playbackSpeed, setPlaybackSpeed] = useState(1);
  const [reviewingLive, setReviewingLive] = useState(false);
  const [cursor, setCursor] = useState(0);
  const [snapshots, setSnapshots] = useState<TwinSnapshot[]>([]);
  const [connected, setConnected] = useState(false);
  const fileRef = useRef<HTMLInputElement>(null);
  const reviewingLiveRef = useRef(false);
  const current = snapshots[Math.min(cursor, snapshots.length - 1)] ?? idle;

  useEffect(() => {
    if (mode !== 'replay' || !playing) return;
    const timer = window.setInterval(() => setCursor((value) => (value + 1) % snapshots.length), 120 / playbackSpeed);
    return () => window.clearInterval(timer);
  }, [mode, playing, playbackSpeed, snapshots.length]);

  useEffect(() => {
    if (mode !== 'live' || !playing || !reviewingLive) return;
    const timer = window.setInterval(() => {
      setCursor((value) => {
        if (value >= snapshots.length - 2) {
          reviewingLiveRef.current = false;
          setReviewingLive(false);
          return Math.max(0, snapshots.length - 1);
        }
        return value + 1;
      });
    }, 50 / playbackSpeed);
    return () => window.clearInterval(timer);
  }, [mode, playing, playbackSpeed, reviewingLive, snapshots.length]);

  useEffect(() => {
    if (mode !== 'live') return;
    let socket: WebSocket | undefined;
    let retryTimer: number | undefined;
    let resetTimer: number | undefined;
    let missionFinished = false;
    let disposed = false;
    setSnapshots([]);
    setCursor(0);
    reviewingLiveRef.current = false;
    setReviewingLive(false);
    const connect = () => {
      if (disposed) return;
      try {
        socket = new WebSocket(WS_URL);
        socket.onopen = () => setConnected(true);
        socket.onclose = () => {
          setConnected(false);
          missionFinished = false;
          if (resetTimer !== undefined) window.clearTimeout(resetTimer);
          setSnapshots([]);
          setCursor(0);
          if (!disposed) retryTimer = window.setTimeout(connect, 500);
        };
        socket.onerror = () => socket?.close();
        socket.onmessage = (event) => {
          try {
            const next = JSON.parse(event.data) as TwinSnapshot;
            if (missionFinished) return;
            setSnapshots((items) => {
              const updated = [...items, next];
              if (!reviewingLiveRef.current) setCursor(updated.length - 1);
              return updated;
            });
            if (next.mission === 'COMPLETE' || next.mission === 'ABORTED' || next.mission === 'FAILED') {
              missionFinished = true;
              resetTimer = window.setTimeout(() => {
                setSnapshots([]);
                setCursor(0);
              }, 900);
            }
          } catch { setConnected(false); }
        };
      } catch {
        setConnected(false);
        retryTimer = window.setTimeout(connect, 500);
      }
    };
    connect();
    return () => {
      disposed = true;
      if (retryTimer !== undefined) window.clearTimeout(retryTimer);
      if (resetTimer !== undefined) window.clearTimeout(resetTimer);
      socket?.close();
    };
  }, [mode]);

  const stale = mode === 'live' && !connected;
  const visibleSnapshots = snapshots.slice(Math.max(0, cursor - 59), cursor + 1);
  const loadReplay = async (file: File) => {
    const text = await file.text();
    const parsed = text.trim().startsWith('[') ? JSON.parse(text) : text.split('\n').filter(Boolean).map((line) => JSON.parse(line));
    setSnapshots(parsed as TwinSnapshot[]); setCursor(0); setMode('replay'); setPlaying(false);
  };
  const togglePlayback = () => {
    if (mode === 'live' && !reviewingLiveRef.current) {
      reviewingLiveRef.current = true;
      setReviewingLive(true);
    }
    setPlaying((value) => !value);
  };
  const cyclePlaybackSpeed = () => setPlaybackSpeed((value) => value === .5 ? 1 : value === 1 ? 2 : value === 2 ? 4 : .5);

  return (
    <main className="app-shell">
      <header className="topbar">
        <div className="brand"><span className="brand-mark"><Box size={17} /></span><div><strong>DIGITAL TWIN</strong><span>{current.source ?? 'HIL'} FLIGHT TESTBED</span></div></div>
        <div className="top-status"><div><StatusDot tone={connected && current.source === 'SIL' ? 'green' : 'red'} /><span>SIL {connected && current.source === 'SIL' ? 'LIVE' : 'OFFLINE'}</span></div><div><StatusDot tone={connected && current.source !== 'SIL' ? 'green' : 'red'} /><span>HIL {connected && current.source !== 'SIL' ? 'LIVE' : 'OFFLINE'}</span></div><div><StatusDot tone={current.fc1.status === 'ONLINE' ? 'green' : 'red'} /><span>FC1</span></div><div><StatusDot tone={current.fc2.status === 'ONLINE' ? 'green' : 'red'} /><span>FC2</span></div></div>
        <div className="timing"><span>LOOP</span><strong>{current.hil.loop_hz.toFixed(1)} <small>Hz</small></strong><i /><span>DEADLINE MISSES</span><strong>{current.hil.deadline_misses}</strong></div>
      </header>
      <section className="workspace">
        <div className="main-column">
          <section className="panel flight-panel">
            <div className="panel-heading"><div><Radio size={14} /><span>FLIGHT VIEW</span><b>LOCAL NED FRAME</b></div><div className="coordinates"><span>X <b>{current.aircraft.x_m.toFixed(1)} m</b></span><span>Y <b>{current.aircraft.y_m.toFixed(1)} m</b></span><span>Z <b>{current.aircraft.z_m.toFixed(1)} m</b></span></div></div>
            <AircraftScene snapshot={current} trail={visibleSnapshots} />
            <div className="attitude-strip"><Metric label="ALTITUDE" value={current.aircraft.altitude_m.toFixed(1)} unit="m" /><Metric label="TARGET" value={current.target.altitude_m.toFixed(1)} unit="m" /><Metric label="PITCH" value={(current.aircraft.pitch_rad * 57.3).toFixed(1)} unit="°" /><Metric label="ROLL" value={(current.aircraft.roll_rad * 57.3).toFixed(1)} unit="°" /><Metric label="AIRSPEED" value={current.aircraft.airspeed_ms.toFixed(1)} unit="m/s" /></div>
          </section>
          <div className="lower-grid">
            <section className="panel avionics-panel"><div className="panel-heading"><div><CircleDot size={14} /><span>AVIONICS HEALTH</span><b>LOGICAL SUBSYSTEMS</b></div></div><AvionicsScene snapshot={current} /></section>
            <section className="panel chart-panel"><div className="panel-heading"><div><Activity size={14} /><span>FLIGHT PARAMETERS</span><b>LAST 60 SAMPLES</b></div></div><TelemetryCharts data={visibleSnapshots} /></section>
          </div>
        </div>
        <aside className="right-column">
          <section className="panel overview-panel">
            <div className="panel-heading"><div><Gauge size={14} /><span>SYSTEM STATE</span></div><span className="sim-time">T+ {current.time_s.toFixed(2)} s</span></div>
            <div className="state-block mission"><span>MISSION</span><strong>{current.mission.replaceAll('_', ' ')}</strong><small>Station hold · target locked</small></div>
            <div className="state-pair"><div><span>HEALTH</span><strong className={current.health === 'HEALTHY' ? 'ok' : 'warn'}><StatusDot tone={current.health === 'HEALTHY' ? 'green' : 'amber'} />{current.health}</strong></div><div><span>SAFETY</span><strong className={current.safety_mode === 'NORMAL' ? 'ok' : 'warn'}>{current.safety_mode}</strong></div></div>
            <div className={`fault-block ${current.active_fault ? 'active' : ''}`}><span>ACTIVE FAULT</span><strong>{current.active_fault ?? 'NONE'}</strong><small>{current.active_fault ? 'Compensation active' : 'No injected or detected fault'}</small></div>
            <div className="actuators"><span>ACTUATORS</span><Metric label="ROTOR" value={current.actuators.rotor_rpm.toFixed(0)} unit="RPM" /><Metric label="LEFT SERVO" value={current.actuators.left_servo_deg.toFixed(1)} unit="°" /><Metric label="RIGHT SERVO" value={current.actuators.right_servo_deg.toFixed(1)} unit="°" /></div>
          </section>
          <section className="panel timeline-panel"><div className="panel-heading"><div><AlertTriangle size={14} /><span>EVENT TIMELINE</span></div><button aria-label="Filter events"><ChevronDown size={15} /></button></div><div className="events">{current.events.slice(-6).reverse().map((event, index) => <div className={`event ${event.level}`} key={`${event.time_s}-${event.type}-${index}`}><time>{event.time_s.toFixed(3)}</time><span>{event.type}</span><p>{event.message}</p></div>)}</div></section>
        </aside>
      </section>
      <footer className="controlbar">
        <div className="mode-switch"><button className={mode === 'live' ? 'active' : ''} onClick={() => setMode('live')}><Radio size={14} />LIVE</button><button className={mode === 'replay' ? 'active' : ''} onClick={() => { setSnapshots(demo); setCursor(0); setMode('replay'); }}><Play size={13} />REPLAY</button></div>
        <button className="icon-button" onClick={togglePlayback}>{playing ? <Pause size={16} /> : <Play size={16} />}</button><button className="load-button" onClick={cyclePlaybackSpeed}>{playbackSpeed}×</button><span className="elapsed">{current.time_s.toFixed(2)} s</span>
        <input className="scrubber" type="range" min="0" max={Math.max(0, snapshots.length - 1)} value={Math.min(cursor, Math.max(0, snapshots.length - 1))} onChange={(event) => { setCursor(Number(event.target.value)); setPlaying(false); if (mode === 'live') { reviewingLiveRef.current = true; setReviewingLive(true); } }} /><span className="duration">{snapshots.at(-1)?.time_s.toFixed(2)} s</span>
        <button className="icon-button" onClick={() => { setCursor(0); setPlaying(false); if (mode === 'live') { reviewingLiveRef.current = true; setReviewingLive(true); } }}><RotateCcw size={15} /></button><button className="load-button" onClick={() => fileRef.current?.click()}><Upload size={14} />LOAD REPLAY</button><input ref={fileRef} hidden type="file" accept=".json,.jsonl" onChange={(event) => event.target.files?.[0] && loadReplay(event.target.files[0])} />
        <div className="source"><StatusDot tone={mode === 'replay' ? 'amber' : stale ? 'red' : 'green'} /><span>{mode === 'replay' ? 'DEMO RECORDING' : connected ? WS_URL : 'WAITING FOR TELEMETRY'}</span></div>
      </footer>
    </main>
  );
}
