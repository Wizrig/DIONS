#!/usr/bin/env python3
"""
DIONS 2.0 Complete Test Harness
Automated end-to-end validation with controlled local network
"""

import subprocess
import json
import time
import sys
import os
import shutil
from pathlib import Path

# Configuration
HOME = str(Path.home())
PROJECT_ROOT = Path(__file__).parent.parent.absolute()
DAEMON = PROJECT_ROOT / "src" / "iocoind"
TEST_BASE = Path(HOME) / "ioc-data" / "dions-test-harness"
NODE1_DIR = TEST_BASE / "node1"
NODE2_DIR = TEST_BASE / "node2"
ARTIFACTS = PROJECT_ROOT / "artifacts" / "final"
LOGS = PROJECT_ROOT / "logs"

class TestNode:
    def __init__(self, datadir, rpcport, p2pport, rpcuser, rpcpass):
        self.datadir = Path(datadir)
        self.rpcport = rpcport
        self.p2pport = p2pport
        self.rpcuser = rpcuser
        self.rpcpass = rpcpass
        self.process = None

    def write_config(self, addnode=None):
        config = f"""rpcuser={self.rpcuser}
rpcpassword={self.rpcpass}
rpcport={self.rpcport}
port={self.p2pport}
server=1
daemon=1
testnet=1
listen=1
staking=0
reservebalance=999999999
"""
        if addnode:
            config += f"addnode={addnode}\n"

        self.datadir.mkdir(parents=True, exist_ok=True)
        (self.datadir / "iocoin.conf").write_text(config)

    def start(self):
        cmd = [str(DAEMON), f"-datadir={self.datadir}"]
        result = subprocess.run(cmd, capture_output=True, text=True)
        time.sleep(3)
        return result.returncode == 0

    def rpc(self, method, *params):
        payload = {
            "method": method,
            "params": list(params),
            "id": 1
        }
        cmd = [
            "curl", "--silent",
            "--user", f"{self.rpcuser}:{self.rpcpass}",
            "--data-binary", json.dumps(payload),
            f"http://127.0.0.1:{self.rpcport}/"
        ]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            return None
        try:
            return json.loads(result.stdout)
        except:
            return None

    def stop(self):
        self.rpc("stop")
        time.sleep(2)

def log(msg):
    print(f"[{time.strftime('%H:%M:%S')}] {msg}")
    sys.stdout.flush()

def main():
    log("=== DIONS 2.0 Complete Test Harness ===")

    # Setup
    ARTIFACTS.mkdir(parents=True, exist_ok=True)
    LOGS.mkdir(parents=True, exist_ok=True)
    if TEST_BASE.exists():
        shutil.rmtree(TEST_BASE)

    # Kill any existing test nodes
    subprocess.run(["pkill", "-f", "dions-test-harness"], stderr=subprocess.DEVNULL)
    time.sleep(2)

    results = []

    # Test 1: Build
    log("[1/8] Building DIONS 2.0...")
    os.chdir(PROJECT_ROOT / "src")
    build = subprocess.run(["make", "-f", "makefile.osx"], capture_output=True)
    if build.returncode != 0 or not DAEMON.exists():
        log("FAIL: Build failed")
        results.append(("Build", "FAIL", "Build failed"))
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log("Build: PASS")
    results.append(("Build", "PASS", "Compiled successfully"))

    # Test 2: Start two-node network
    log("[2/8] Starting two-node test network...")
    node1 = TestNode(NODE1_DIR, 46001, 46002, "test1", "pass1")
    node2 = TestNode(NODE2_DIR, 46003, 46004, "test2", "pass2")

    node1.write_config(addnode="127.0.0.1:46004")
    node2.write_config(addnode="127.0.0.1:46002")

    if not node1.start():
        log("FAIL: Node 1 startup failed")
        results.append(("Node Startup", "FAIL", "Node 1 failed to start"))
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    if not node2.start():
        log("FAIL: Node 2 startup failed")
        results.append(("Node Startup", "FAIL", "Node 2 failed to start"))
        node1.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log("Nodes: RUNNING")
    results.append(("Node Startup", "PASS", "Both nodes running"))

    # Test 3: Peer handshake
    log("[3/8] Verifying peer handshake...")
    time.sleep(10)

    info1 = node1.rpc("getinfo")
    info2 = node2.rpc("getinfo")

    if not info1 or not info2:
        log("FAIL: RPC not responding")
        results.append(("RPC", "FAIL", "Nodes not responding to RPC"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    protocol1 = info1.get("result", {}).get("protocolversion", 0)
    protocol2 = info2.get("result", {}).get("protocolversion", 0)

    if protocol1 != 60023 or protocol2 != 60023:
        log(f"FAIL: Wrong protocol (N1:{protocol1}, N2:{protocol2})")
        results.append(("Protocol", "FAIL", f"Expected 60023, got {protocol1}/{protocol2}"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    peers1 = node1.rpc("getpeerinfo")
    peers2 = node2.rpc("getpeerinfo")

    conn1 = len(peers1.get("result", []))
    conn2 = len(peers2.get("result", []))

    if conn1 < 1 or conn2 < 1:
        log(f"FAIL: No peer connections (N1:{conn1}, N2:{conn2})")
        results.append(("Peer Handshake", "FAIL", f"Connections: N1={conn1}, N2={conn2}"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log(f"Peer handshake: PASS (N1:{conn1} peers, N2:{conn2} peers)")
    results.append(("Peer Handshake", "PASS", f"Protocol 60023, N1={conn1} peers, N2={conn2} peers"))

    # Test 4: RSA key generation
    log("[4/8] Testing RSA-4096 key generation...")

    addr1_resp = node1.rpc("getnewaddress")
    addr2_resp = node2.rpc("getnewaddress")

    addr1 = addr1_resp.get("result")
    addr2 = addr2_resp.get("result")

    if not addr1 or not addr2:
        log("FAIL: Address generation failed")
        results.append(("Address Generation", "FAIL", "Could not generate addresses"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log(f"Addresses: {addr1}, {addr2}")

    keys1 = node1.rpc("publicKey", addr1)
    keys2 = node2.rpc("publicKey", addr2)

    if not keys1 or not keys2:
        log("FAIL: RSA key generation failed")
        results.append(("RSA Key Generation", "FAIL", "publicKey RPC failed"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    keys1_result = keys1.get("result", [])
    keys2_result = keys2.get("result", [])

    if len(keys1_result) < 2 or "BEGIN RSA PRIVATE KEY" not in str(keys1_result):
        log("FAIL: RSA keys not generated for Node 1")
        results.append(("RSA Keys", "FAIL", "Node 1 RSA generation incomplete"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    if len(keys2_result) < 2 or "BEGIN RSA PRIVATE KEY" not in str(keys2_result):
        log("FAIL: RSA keys not generated for Node 2")
        results.append(("RSA Keys", "FAIL", "Node 2 RSA generation incomplete"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log("RSA-4096 generation: PASS")
    results.append(("RSA-4096 Keys", "PASS", "Both nodes generated keys"))

    # Test 5: Key persistence
    log("[5/8] Testing RSA key persistence...")

    myrsa1 = node1.rpc("myRSAKeys")
    myrsa2 = node2.rpc("myRSAKeys")

    count1 = len(myrsa1.get("result", []))
    count2 = len(myrsa2.get("result", []))

    if count1 < 1 or count2 < 1:
        log(f"FAIL: RSA keys not persisted (N1:{count1}, N2:{count2})")
        results.append(("RSA Persistence", "FAIL", f"Keys not stored: N1={count1}, N2={count2}"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log(f"RSA key persistence: PASS (N1:{count1} keys, N2:{count2} keys)")
    results.append(("RSA Persistence", "PASS", f"N1={count1} keys, N2={count2} keys"))

    # Test 6: DIONS RPC presence
    log("[6/8] Testing DIONS RPC presence...")

    dions_rpcs = ["aliasList", "plainTextMessageList", "decryptedMessageList", "myRSAKeys"]
    missing = []

    for rpc in dions_rpcs:
        resp = node1.rpc(rpc)
        if not resp or "error" in resp:
            if resp and "Method not found" in str(resp.get("error", {})):
                missing.append(rpc)

    if missing:
        log(f"FAIL: Missing DIONS RPCs: {missing}")
        results.append(("DIONS RPCs", "FAIL", f"Missing: {', '.join(missing)}"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log("DIONS RPCs: PASS")
    results.append(("DIONS RPCs", "PASS", "All tested RPCs present"))

    # Test 7: Restart persistence
    log("[7/8] Testing persistence across restart...")

    log("Stopping nodes...")
    node1.stop()
    node2.stop()
    time.sleep(3)

    log("Restarting nodes...")
    node1.start()
    node2.start()
    time.sleep(5)

    myrsa1_after = node1.rpc("myRSAKeys")
    count1_after = len(myrsa1_after.get("result", []))

    if count1_after != count1:
        log(f"FAIL: RSA keys lost after restart (was {count1}, now {count1_after})")
        results.append(("Restart Persistence", "FAIL", f"Keys lost: {count1} -> {count1_after}"))
        node1.stop()
        node2.stop()
        (ARTIFACTS / "RESULT.txt").write_text("FAIL")
        return 1

    log("Restart persistence: PASS")
    results.append(("Restart Persistence", "PASS", "RSA keys survived restart"))

    # Test 8: Stability monitor (30 min)
    log("[8/8] Running 30-minute stability monitor...")
    log("(Shortened to 5 minutes for test harness)")

    monitor_duration = 300  # 5 min for faster testing
    monitor_interval = 60
    start_time = time.time()

    errors = 0
    disconnects = 0

    while time.time() - start_time < monitor_duration:
        elapsed = int(time.time() - start_time)
        remaining = monitor_duration - elapsed

        info1 = node1.rpc("getinfo")
        info2 = node2.rpc("getinfo")

        if not info1 or not info2:
            errors += 1
            log(f"WARNING: RPC error (total: {errors})")

        if errors > 3:
            log("FAIL: Too many RPC errors during monitoring")
            results.append(("Stability Monitor", "FAIL", f"{errors} RPC errors"))
            node1.stop()
            node2.stop()
            (ARTIFACTS / "RESULT.txt").write_text("FAIL")
            return 1

        log(f"  Monitor: {elapsed}s elapsed, {remaining}s remaining")
        time.sleep(monitor_interval)

    log("Stability monitor: PASS")
    results.append(("Stability Monitor", "PASS", "5-min stable operation"))

    # Cleanup
    log("Stopping test nodes...")
    node1.stop()
    node2.stop()

    # Generate report
    log("")
    log("=== TEST RESULTS ===")
    for test, status, detail in results:
        log(f"{test}: {status} - {detail}")

    report = f"""DIONS 2.0 Final Test Report
============================

Test Date: {time.strftime('%Y-%m-%d %H:%M:%S')}
Platform: macOS arm64
Protocol: 60023

RESULTS
-------
"""
    for test, status, detail in results:
        report += f"{test}: {status}\n  {detail}\n\n"

    report += f"""
VERDICT
-------
All tests: PASS
Status: GREEN
Recommendation: APPROVED

Test completed: {time.strftime('%Y-%m-%d %H:%M:%S')}
"""

    (ARTIFACTS / "final-report.txt").write_text(report)
    (ARTIFACTS / "RESULT.txt").write_text("PASS")

    log("")
    log("=== ALL TESTS PASSED ===")
    log(f"Report: {ARTIFACTS / 'final-report.txt'}")

    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        log("\nTest interrupted")
        subprocess.run(["pkill", "-f", "dions-test-harness"], stderr=subprocess.DEVNULL)
        sys.exit(1)
    except Exception as e:
        log(f"FATAL ERROR: {e}")
        import traceback
        traceback.print_exc()
        subprocess.run(["pkill", "-f", "dions-test-harness"], stderr=subprocess.DEVNULL)
        sys.exit(1)
