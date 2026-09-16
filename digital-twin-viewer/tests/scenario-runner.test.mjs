import assert from 'node:assert/strict';
import test from 'node:test';
import { createServer } from 'vite';
import { scenarioRunner } from '../server/scenario-runner.ts';

void test('local runner catalog, validation, concurrency and stop lifecycle', async () => {
  const server = await createServer({ configFile: false, plugins: [scenarioRunner()], server: { host: '127.0.0.1', port: 0 } });
  await server.listen();
  const port = server.httpServer.address().port;
  const url = `http://127.0.0.1:${port}/api/scenarios`;
  const post = (body, headers = {}) => fetch(url, { method: 'POST', headers: { 'Content-Type': 'application/json', ...headers }, body: JSON.stringify(body) });
  try {
    const initial = await (await fetch(url)).json();
    assert.ok(initial.scenarios.some((item) => item.id === 'NOMINAL-001'));
    assert.equal(initial.run.status, 'idle');
    assert.equal((await post({ scenario: 'NOMINAL-001', interface: 'loopback' }, { Origin: 'http://untrusted.test' })).status, 403);
    assert.equal((await post({ scenario: '; touch /tmp/unwanted', interface: 'loopback' })).status, 400);
    assert.equal((await post({ scenario: 'NOMINAL-001', interface: 'loopback' })).status, 202);
    assert.equal((await post({ scenario: 'NOMINAL-001', interface: 'loopback' })).status, 409);
    const running = await (await fetch(url)).json();
    assert.equal(running.run.status, 'running');
    assert.match(running.run.command, /--interface loopback/);
    assert.equal((await fetch(url, { method: 'DELETE' })).status, 200);
    let finished;
    for (let attempt = 0; attempt < 50; attempt++) {
      finished = await (await fetch(url)).json();
      if (finished.run.status === 'stopped') break;
      await new Promise((resolve) => setTimeout(resolve, 100));
    }
    assert.equal(finished.run.status, 'stopped');
    assert.ok(finished.run.output.length > 0);
  } finally {
    await fetch(url, { method: 'DELETE' });
    await server.close();
  }
});
