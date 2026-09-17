import { execFile, spawn, type ChildProcess } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import { promisify } from 'node:util';
import type { IncomingMessage, ServerResponse } from 'node:http';
import type { Plugin } from 'vite';

const root = fileURLToPath(new URL('../../', import.meta.url));
const executable = `${root}artifacts/linux/hil_runner`;
const execute = promisify(execFile);

export function scenarioRunner(): Plugin {
  let child: ChildProcess | undefined;
  let stopping: ReturnType<typeof setTimeout> | undefined;
  let run = { status: 'idle', scenario: '', command: '', output: '', exitCode: null as number | null };
  const append = (text: string) => { run.output = (run.output + text).slice(-64_000); };
  const stop = () => {
    if (!child || run.status === 'stopping') return;
    run.status = 'stopping';
    const active = child;
    active.kill('SIGTERM');
    stopping = setTimeout(() => active.kill('SIGKILL'), 3000);
    stopping.unref();
  };
  const catalog = async () => {
    const { stdout } = await execute(executable, ['--list'], { cwd: root, timeout: 5000, maxBuffer: 256_000 });
    return [...stdout.matchAll(/^\s+([\w-]+)\s+:\s+(.+)$/gm)].map((match) => ({ id: match[1], description: match[2] }));
  };
  const respond = (res: ServerResponse, status: number, data: unknown) => {
    res.writeHead(status, { 'Content-Type': 'application/json', 'Cache-Control': 'no-store' });
    res.end(JSON.stringify(data));
  };
  const handle = async (req: IncomingMessage, res: ServerResponse) => {
    const host = req.headers.host ?? '';
    const address = req.socket.remoteAddress;
    if (!['127.0.0.1', '::1', '::ffff:127.0.0.1'].includes(address ?? '') ||
        !/^(localhost|127\.0\.0\.1|\[::1\])(:\d+)?$/.test(host) ||
        (req.headers.origin && req.headers.origin !== `http://${host}`) ||
        req.headers['sec-fetch-site'] === 'cross-site') {
      respond(res, 403, { error: 'Local same-origin requests only.' });
      return;
    }
    if (req.method === 'GET') {
      respond(res, 200, { scenarios: await catalog(), run });
      return;
    }
    if (req.method === 'DELETE') {
      stop();
      respond(res, 200, { run });
      return;
    }
    if (req.method === 'PATCH') {
      run.output = '';
      respond(res, 200, { run });
      return;
    }
    if (req.method !== 'POST') { respond(res, 405, { error: 'Method not allowed.' }); return; }
    if (!req.headers['content-type']?.startsWith('application/json')) { respond(res, 415, { error: 'JSON required.' }); return; }
    let body = '';
    for await (const chunk of req) {
      body += chunk;
      if (body.length > 2048) { respond(res, 413, { error: 'Request too large.' }); return; }
    }
    let input;
    try { input = JSON.parse(body); } catch { respond(res, 400, { error: 'Invalid JSON.' }); return; }
    const scenarios = await catalog();
    if (!input || !scenarios.some((item) => item.id === input.scenario) || !['auto', 'loopback'].includes(input.interface)) {
      respond(res, 400, { error: 'Unknown scenario or interface.' }); return;
    }
    if (child) { respond(res, 409, { error: 'A scenario is already running.' }); return; }
    const args = ['--scenario', input.scenario, '--interface', input.interface];
    run = { status: 'running', scenario: input.scenario, command: `./artifacts/linux/hil_runner ${args.join(' ')}`, output: '', exitCode: null };
    child = spawn(executable, args, { cwd: root, stdio: ['ignore', 'pipe', 'pipe'], env: { ...process.env, NO_COLOR: '1' } });
    child.stdout?.setEncoding('utf8').on('data', append);
    child.stderr?.setEncoding('utf8').on('data', append);
    child.on('error', (error) => { append(error.message); run.status = 'failed'; });
    child.on('close', (code, signal) => {
      run.status = run.status === 'stopping' ? 'stopped' : code === 0 ? 'completed' : 'failed';
      run.exitCode = code;
      if (signal) append(`\nProcess terminated: ${signal}\n`);
      clearTimeout(stopping);
      child = undefined;
    });
    respond(res, 202, { run });
  };
  return {
    name: 'local-scenario-runner',
    configureServer(server) {
      server.middlewares.use('/api/scenarios', (req, res) => {
        void handle(req, res).catch((error: Error) => respond(res, 500, { error: `HIL runner unavailable: ${error.message}` }));
      });
      server.httpServer?.once('close', stop);
    },
  };
}
