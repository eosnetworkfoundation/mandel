#!/usr/bin/env python3

from testUtils import Utils
from Cluster import Cluster
from WalletMgr import WalletMgr
from TestHelper import TestHelper

import random
import os
import subprocess
import tempfile
import signal
import time

###############################################################
# deep_mind_test.py
#
# This tests deep mind logging via FIFO related functionalities:
#   * FIFO exists before nodeos starts
#   * FIFO does not exist before nodeos starts
#
###############################################################

Print=Utils.Print
errorExit=Utils.errorExit

args=TestHelper.parse_args({"--keep-logs" ,"--dump-error-details","-v","--leave-running","--clean-run"})
debug=args.v
killEosInstances= not args.leave_running
dumpErrorDetails=args.dump_error_details
keepLogs=args.keep_logs
killAll=args.clean_run

seed=1
Utils.Debug=debug
testSuccessful=False

random.seed(seed) # Use a fixed seed for repeatability.
cluster=Cluster(walletd=True)
walletMgr=WalletMgr(True)

pnodes=1
total_nodes=pnodes

try:
    TestHelper.printSystemInfo("BEGIN")

    cluster.setWalletMgr(walletMgr)

    cluster.killall(allInstances=killAll)
    cluster.cleanup()
    walletMgr.killall(allInstances=killAll)
    walletMgr.cleanup()

    tmpdir = tempfile.mkdtemp()
    fifoFile = os.path.join(tmpdir, 'fifo')

    # Test 1: FIFO exists before nodeos starts
    os.mkfifo(fifoFile)

    specificExtraNodeosArgs={}
    # A non-existing logging.json makes nodeos initializing deep mind at startup
    fakeLoggingJson = os.path.join(tmpdir, 'logging.json')
    specificExtraNodeosArgs[0]=f' --logconf {fakeLoggingJson} --deep-mind-fifo {fifoFile} '

    extraNodeosArgs=f' --plugin eosio::trace_api_plugin --trace-no-abis '

    Print("Stand up cluster")
    if cluster.launch(pnodes=pnodes, totalNodes=total_nodes, extraNodeosArgs=extraNodeosArgs, specificExtraNodeosArgs=specificExtraNodeosArgs) is False:
       errorExit("Failed to stand up eos cluster.")

    Print ("Wait for Cluster stabilization")
    # wait for cluster to start producing blocks
    if not cluster.waitOnClusterBlockNumSync(3):
       errorExit("Cluster never stabilized")
    Print ("Cluster stabilized")

    # FIFO should have deep mind logs, each of which starts with DMLOG
    with open(fifoFile, "r") as f:
       if 'DMLOG' not in f.readline():
          errorExit("No deep mind logging found in FIFO exists before nodeos starts test")
    Print ("Test 1 (FIFO exists before nodeos start succeeded)") 

    # Test 2: FIFO does not exist before nodeos starts
    node = cluster.getNode(0)
    node.kill(signal.SIGTERM)
    # remove FIFO file so that no FIFO exists before nodeos starts
    os.remove(fifoFile)
    node.relaunch()
    time.sleep(3)
    with open(fifoFile, "r") as f:
       if 'DMLOG' not in f.readline():
          errorExit("No deep mind logging found in FIFO does not exist before nodeos starts test")
    Print ("Test 2 (FIFO does not exist before nodeos start succeeded)") 

    testSuccessful=True
finally:
    TestHelper.shutdown(cluster, walletMgr, testSuccessful=testSuccessful, killEosInstances=killEosInstances, killWallet=killEosInstances, keepLogs=keepLogs, cleanRun=killAll, dumpErrorDetails=dumpErrorDetails)

exitCode = 0 if testSuccessful else 1
exit(exitCode)
