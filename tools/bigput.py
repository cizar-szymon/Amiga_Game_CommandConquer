# Upload a very large file through AmiAgent with a long socket timeout (amiga_remote.py waits only 30 s for the reply).
import sys, time
sys.path.insert(0, "tools")
import amiga_remote as ar
cfg, local, remote = sys.argv[1], sys.argv[2], sys.argv[3]
class A: pass
a = A(); a.config = cfg; a.host = a.port = a.password = None
host, port, pw = ar.load_settings(a)
agent = ar.Agent(host, port, pw, timeout=1800)
t = time.time()
agent.put(open(local, "rb").read(), remote)
print("done", round(time.time() - t), "s")
