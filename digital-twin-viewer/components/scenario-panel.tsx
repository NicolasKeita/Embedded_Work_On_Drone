'use client';

import { useEffect, useState } from 'react';
import { Play, Square, Terminal, Trash2 } from 'lucide-react';

type Run = { status: string; scenario: string; command: string; output: string; exitCode: number | null };
type Catalog = { scenarios: { id: string; description: string }[]; run: Run };

export function ScenarioPanel({ onStart }: { onStart: () => void }) {
  const [catalog, setCatalog] = useState<Catalog | null>(null);
  const [scenario, setScenario] = useState('NOMINAL-001');
  const [target, setTarget] = useState('auto');
  const [error, setError] = useState('');
  const [pending, setPending] = useState(false);
  useEffect(() => {
    let disposed = false;
    let timer: ReturnType<typeof setTimeout>;
    const refresh = async () => {
      try {
        const response = await fetch('/api/scenarios');
        const data = await response.json() as Catalog & { error?: string };
        if (!response.ok) throw new Error(data.error);
        if (!disposed) { setCatalog(data); setError(''); }
      } catch (cause) {
        if (!disposed) setError(cause instanceof Error ? cause.message : 'Local runner unavailable.');
      } finally {
        if (!disposed) timer = setTimeout(refresh, 1000);
      }
    };
    void refresh();
    return () => { disposed = true; clearTimeout(timer); };
  }, []);
  const act = async (method: 'POST' | 'DELETE' | 'PATCH') => {
    setPending(true);
    setError('');
    try {
      const response = await fetch('/api/scenarios', {
        method,
        headers: { 'Content-Type': 'application/json' },
        ...(method === 'POST' ? { body: JSON.stringify({ scenario, interface: target }) } : {}),
      });
      const data = await response.json() as Catalog & { error?: string };
      if (!response.ok) throw new Error(data.error);
      setCatalog((previous) => previous && { ...previous, run: data.run });
      if (method === 'POST') onStart();
    } catch (cause) {
      setError(cause instanceof Error ? cause.message : 'Request failed.');
    } finally { setPending(false); }
  };
  const active = catalog?.run.status === 'running' || catalog?.run.status === 'stopping';
  return (
    <section className="panel scenario-panel" id="scenarios">
      <div className="panel-heading"><div><Terminal size={14} /><span>Scenario runner</span></div><b>LOCAL HIL</b></div>
      <div className="scenario-content">
        <label htmlFor="scenario-id">SCENARIO</label>
        <select id="scenario-id" value={scenario} disabled={active || pending} onChange={(event) => setScenario(event.target.value)}>
          {!catalog?.scenarios.length && <option value="NOMINAL-001">NOMINAL-001</option>}
          {[['NOMINAL-', 'Nominal'], ['WIND-', 'Vent'], ['FAULT_INJECTOR-', 'Fault injection']].map(([prefix, label]) =>
            <optgroup key={prefix} label={label}>{catalog?.scenarios.filter((item) => item.id.startsWith(prefix)).map((item) => <option key={item.id} value={item.id}>{item.id}</option>)}</optgroup>)}
        </select>
        <p>{catalog?.scenarios.find((item) => item.id === scenario)?.description ?? 'Loading the local runner catalog…'}</p>
        <label htmlFor="scenario-target">INTERFACE</label>
        <select id="scenario-target" value={target} disabled={active || pending} onChange={(event) => setTarget(event.target.value)}>
          <option value="auto">Auto · trusted FC1 / loopback fallback</option>
          <option value="loopback">Loopback · host emulator</option>
        </select>
        <div className="scenario-actions">
          <button className="load-button scenario-start" disabled={!catalog || active || pending} onClick={() => void act('POST')}><Play size={14} />RUN SCENARIO</button>
          <button className="load-button" disabled={!active || pending || catalog?.run.status === 'stopping'} onClick={() => void act('DELETE')}><Square size={13} />STOP</button>
          <button className="load-button" disabled={!catalog?.run.output || pending} onClick={() => void act('PATCH')}><Trash2 size={13} />CLEAN</button>
        </div>
        <output className="scenario-status">{catalog?.run.status.toUpperCase() ?? 'CONNECTING'}{catalog?.run.scenario && ` · ${catalog.run.scenario}`}{catalog?.run.exitCode != null && ` · exit ${catalog.run.exitCode}`}</output>
        {error && <p role="alert" className="scenario-error">{error}</p>}
        {catalog?.run.command && <code className="scenario-command">{catalog.run.command}</code>}
        <pre className="scenario-output" aria-label="Runner output">{catalog?.run.output || 'Runner output will appear here.'}</pre>
      </div>
    </section>
  );
}
