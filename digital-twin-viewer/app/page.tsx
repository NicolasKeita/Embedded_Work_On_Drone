'use client';

import { useCallback, useEffect, useMemo, useRef, useState, useSyncExternalStore } from 'react';
import { AlertTriangle, Box, CircleDot, Gauge, Pause, Play, Radio, RotateCcw, Upload } from 'lucide-react';
import { AircraftSceneContent, AircraftSceneOverlay } from '@/components/aircraft-scene';
import { AvionicsSceneContent, AvionicsSceneDom, type AvionicsPanelRefs } from '@/components/avionics-scene';
import { ScenarioPanel } from '@/components/scenario-panel';
import { TwinCanvas } from '@/components/twin-canvas';
import { createDemoSnapshots, createIdleSnapshot, hasAltitudeFault, holdLastKnownAltitude, holdLastKnownAltitudes, isFaultDetectionPending, type TwinSnapshot } from '@/lib/twin-data';

const WS_URL = 'ws://localhost:8765/twin';
const WS_RETRY_INITIAL_DELAY_MS = 1_000;
const WS_RETRY_MAX_DELAY_MS = 8_000;
const subscribeToClientRender = () => () => {};
const clientSnapshot = () => true;
const serverSnapshot = () => false;

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
  const scenesReady = useSyncExternalStore(subscribeToClientRender, clientSnapshot, serverSnapshot);
  const fileRef = useRef<HTMLInputElement>(null);
  const reviewingLiveRef = useRef(false);
  const current = snapshots[Math.min(cursor, snapshots.length - 1)] ?? idle;
  const faultDetectionPending = isFaultDetectionPending(current);
  const aircraftRef = useRef<HTMLDivElement>(null);
  const [aircraftDom, setAircraftDom] = useState<HTMLDivElement | undefined>(undefined);
  const aircraftCallbackRef = useCallback((node: HTMLDivElement | null) => { aircraftRef.current = node; setAircraftDom(node ?? undefined); }, []);
  const airframeRef = useRef<HTMLDivElement>(null);
  const [airframeDom, setAirframeDom] = useState<HTMLDivElement | undefined>(undefined);
  const airframeCallbackRef = useCallback((node: HTMLDivElement | null) => { airframeRef.current = node; setAirframeDom(node ?? undefined); }, []);
  const fc1Ref = useRef<HTMLDivElement>(null);
  const [fc1Dom, setFc1Dom] = useState<HTMLDivElement | undefined>(undefined);
  const fc1CallbackRef = useCallback((node: HTMLDivElement | null) => { fc1Ref.current = node; setFc1Dom(node ?? undefined); }, []);
  const fc2Ref = useRef<HTMLDivElement>(null);
  const [fc2Dom, setFc2Dom] = useState<HTMLDivElement | undefined>(undefined);
  const fc2CallbackRef = useCallback((node: HTMLDivElement | null) => { fc2Ref.current = node; setFc2Dom(node ?? undefined); }, []);
  const avionicsRefs: AvionicsPanelRefs = {
    airframe: { ref: airframeRef, domElement: airframeDom, callbackRef: airframeCallbackRef },
    fc1: { ref: fc1Ref, domElement: fc1Dom, callbackRef: fc1CallbackRef },
    fc2: { ref: fc2Ref, domElement: fc2Dom, callbackRef: fc2CallbackRef },
  };

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
    let retryDelayMs = WS_RETRY_INITIAL_DELAY_MS;
    let missionFinished = false;
    let disposed = false;
    setSnapshots([]);
    setCursor(0);
    reviewingLiveRef.current = false;
    setReviewingLive(false);
    const scheduleReconnect = () => {
      if (disposed) return;
      retryTimer = window.setTimeout(connect, retryDelayMs);
      retryDelayMs = Math.min(retryDelayMs * 2, WS_RETRY_MAX_DELAY_MS);
    };
    const connect = () => {
      if (disposed) return;
      try {
        socket = new WebSocket(WS_URL);
        socket.onopen = () => {
          retryDelayMs = WS_RETRY_INITIAL_DELAY_MS;
          setConnected(true);
        };
        socket.onclose = () => {
          setConnected(false);
          missionFinished = false;
          if (resetTimer !== undefined) window.clearTimeout(resetTimer);
          setSnapshots([]);
          setCursor(0);
          scheduleReconnect();
        };
        socket.onerror = () => socket?.close();
        socket.onmessage = (event) => {
          try {
            const next = JSON.parse(event.data) as TwinSnapshot;
            if (missionFinished) return;
            setSnapshots((items) => {
              const updated = [...items, holdLastKnownAltitude(next, items.at(-1))];
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
        scheduleReconnect();
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
    setSnapshots(holdLastKnownAltitudes(parsed as TwinSnapshot[])); setCursor(0); setMode('replay'); setPlaying(false);
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
      <nav className="workspace-rail" aria-label="Workspace sections">
        <a className="rail-logo" href="#flight" aria-label="Digital Twin flight view"><Box size={25} /></a>
        <a href="#flight"><Radio size={21} /><span>Flight</span></a>
        <a href="#airframe"><CircleDot size={21} /><span>Airframe</span></a>
        <a href="#system"><Gauge size={21} /><span>System</span></a>
        <a href="#scenarios"><Play size={21} /><span>Scenarios</span></a>
        <a href="#events"><AlertTriangle size={21} /><span>Events</span></a>
        <span className="rail-caption">DIGITAL TWIN / HIL + SIL</span>
      </nav>
      <header className="topbar">
        <div className="brand"><span className="brand-mark"><Box size={17} /></span><div><strong>DIGITAL TWIN</strong><span>{current.source ?? 'HIL'} FLIGHT TESTBED</span></div></div>
        <div className="top-status"><div><StatusDot tone={connected && current.source === 'SIL' ? 'green' : 'red'} /><span>SIL {connected && current.source === 'SIL' ? 'LIVE' : 'OFFLINE'}</span></div><div><StatusDot tone={connected && current.source !== 'SIL' ? 'green' : 'red'} /><span>HIL {connected && current.source !== 'SIL' ? 'LIVE' : 'OFFLINE'}</span></div><div><StatusDot tone={current.fc1.status === 'ONLINE' ? 'green' : 'red'} /><span>FC1</span></div><div><StatusDot tone={current.fc2.status === 'ONLINE' ? 'green' : 'red'} /><span>FC2</span></div></div>
        <div className="timing"><span>LOOP</span><strong>{current.hil.loop_hz.toFixed(1)} <small>Hz</small></strong><i /><span>DEADLINE MISSES</span><strong>{current.hil.deadline_misses}</strong></div>
        <section className="header-controls" aria-label="Telemetry source">
        <div className="mode-switch"><button className={mode === 'live' ? 'active' : ''} onClick={() => setMode('live')}><Radio size={14} />LIVE</button><button className={mode === 'replay' ? 'active' : ''} onClick={() => { setSnapshots(demo); setCursor(0); setMode('replay'); }}><Play size={13} />REPLAY</button></div>
        <button className="load-button" onClick={() => fileRef.current?.click()}><Upload size={14} />LOAD REPLAY</button><input ref={fileRef} hidden type="file" accept=".json,.jsonl" onChange={(event) => event.target.files?.[0] && loadReplay(event.target.files[0])} />
        </section>
      </header>
      {scenesReady && (
        <TwinCanvas>
          <AircraftSceneContent track={aircraftRef} domElement={aircraftDom} snapshot={current} trail={visibleSnapshots} />
          <AvionicsSceneContent refs={avionicsRefs} snapshot={current} />
        </TwinCanvas>
      )}
      <section className="workspace">
        <div className="main-column">
          <section className="telemetry-strip" aria-label="Flight measurements">
            <Metric label={hasAltitudeFault(current) ? 'ALTITUDE · HELD' : 'ALTITUDE'} value={current.aircraft.altitude_m.toFixed(1)} unit="m" />
            <Metric label="AIRSPEED" value={current.aircraft.airspeed_ms.toFixed(1)} unit="m/s" />
            <Metric label="PITCH" value={(current.aircraft.pitch_rad * 57.3).toFixed(1)} unit="°" />
            <Metric label="ROLL" value={(current.aircraft.roll_rad * 57.3).toFixed(1)} unit="°" />
          </section>
          <section className="panel flight-panel" id="flight">
            <div className="panel-heading"><div><Radio size={14} /><span>Flight view</span><b>LOCAL NED FRAME</b></div><div className="coordinates"><span>X <b>{current.aircraft.x_m.toFixed(1)} m</b></span><span>Y <b>{current.aircraft.y_m.toFixed(1)} m</b></span><span>Z <b>{current.aircraft.z_m.toFixed(1)} m</b></span></div></div>
            {scenesReady
              ? <div ref={aircraftCallbackRef} className="scene-canvas"><AircraftSceneOverlay snapshot={current} /></div>
              : <div className="scene-canvas" />}
            <div className="viewer-controls">
              <button className="icon-button" aria-label={playing ? 'Pause playback' : 'Resume playback'} onClick={togglePlayback}>{playing ? <Pause size={16} /> : <Play size={16} />}</button>
              <button className="load-button" aria-label={`Playback speed: ${playbackSpeed}×. Click to change`} onClick={cyclePlaybackSpeed}>{playbackSpeed}×</button>
              <span className="elapsed">{current.time_s.toFixed(2)} s</span>
              <input className="scrubber" aria-label="Playback position" type="range" min="0" max={Math.max(0, snapshots.length - 1)} value={Math.min(cursor, Math.max(0, snapshots.length - 1))} onChange={(event) => { setCursor(Number(event.target.value)); setPlaying(false); if (mode === 'live') { reviewingLiveRef.current = true; setReviewingLive(true); } }} />
              <span className="duration">{(snapshots.at(-1)?.time_s ?? 0).toFixed(2)} s</span>
              <button className="icon-button" aria-label="Restart playback" onClick={() => { setCursor(0); setPlaying(false); if (mode === 'live') { reviewingLiveRef.current = true; setReviewingLive(true); } }}><RotateCcw size={15} /></button>
            </div>
          </section>
          <div className="lower-grid">
            <section className="panel avionics-panel" id="airframe"><div className="panel-heading"><div><CircleDot size={14} /><span>Airframe & avionics</span><b>FAULT LOCALIZATION</b></div></div>{scenesReady ? <AvionicsSceneDom refs={avionicsRefs} /> : <div className="avionics-canvas" />}</section>
          </div>
        </div>
        <aside className="right-column">
          <ScenarioPanel onStart={() => { setMode('live'); setPlaying(true); setSnapshots([]); setCursor(0); reviewingLiveRef.current = false; setReviewingLive(false); }} />
          <section className="panel overview-panel" id="system">
            <div className="panel-heading"><div><Gauge size={14} /><span>System state</span></div><span className="sim-time">T+ {current.time_s.toFixed(2)} s</span></div>
            <div className="state-block mission"><span className="mission-label">MISSION STATUS <span>↗</span></span><strong>{current.mission.replaceAll('_', ' ')}</strong><small>{stale ? 'Waiting for flight data' : mode === 'replay' ? 'Reviewing recorded flight data' : 'Receiving flight telemetry'}</small></div>

            <div className="state-pair"><div><span>HEALTH</span><strong className={current.health === 'HEALTHY' && !faultDetectionPending ? 'ok' : 'warn'}><StatusDot tone={current.health === 'HEALTHY' && !faultDetectionPending ? 'green' : 'amber'} />{faultDetectionPending ? `${current.health} · DETECTION PENDING` : current.health}</strong></div><div><span>SAFETY</span><strong className={current.safety_mode === 'NORMAL' && !faultDetectionPending ? 'ok' : 'warn'}>{faultDetectionPending ? `${current.safety_mode} · RESPONSE PENDING` : current.safety_mode}</strong></div></div>
            <div className={`fault-block ${current.active_fault ? 'active' : ''}`}><span>ACTIVE FAULT</span><strong>{current.active_fault ?? 'NONE'}</strong><small>{hasAltitudeFault(current) ? 'Barometer unavailable · altitude held at last known value' : faultDetectionPending ? 'Injected fault · awaiting FC2 detection' : current.active_fault ? 'Safety response active' : 'No injected or detected fault'}</small></div>
            <div className="actuators"><span>ACTUATORS</span><Metric label="ROTOR" value={current.actuators.rotor_rpm.toFixed(0)} unit="RPM" /><Metric label="LEFT SERVO" value={current.actuators.left_servo_deg.toFixed(1)} unit="°" /><Metric label="RIGHT SERVO" value={current.actuators.right_servo_deg.toFixed(1)} unit="°" /></div>
          </section>
          <section className="panel timeline-panel" id="events"><div className="panel-heading"><div><AlertTriangle size={14} /><span>Event timeline</span></div><span className="event-count">{current.events.length}</span></div><div className="events">{current.events.length === 0 && <div className="empty-events"><CircleDot size={24} /><strong>All quiet on the timeline</strong><p>Flight events will appear here as telemetry arrives.</p></div>}{current.events.slice(-6).reverse().map((event, index) => <div className={`event ${event.level}`} key={`${event.time_s}-${event.type}-${index}`}><time>{event.time_s.toFixed(3)}</time><span>{event.type}</span><p>{event.message}</p></div>)}</div></section>
        </aside>
      </section>

    </main>
  );
}
