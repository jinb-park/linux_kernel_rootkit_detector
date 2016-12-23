import logging
import time
import struct, fcntl, os
from daemon import runner
from lkrd_tool import inspect
import array

class lkrd_daemon():

    def __init__(self):
        self.stdin_path = '/dev/null'
        self.stdout_path = '/dev/tty'
        self.stderr_path = '/dev/tty'
        self.pidfile_path =  '/var/run/lkrd_daemon.pid'
        self.lkrd_device_path = '/dev/lkrd_device'
        self.pidfile_timeout = 5
        self.cmd_get_inspect_lkm = 0
        self.cmd_send_inspect_result = 1

    def run(self):
        while True:
            device = open(self.lkrd_device_path)

            buf = array.array('c', '\0' * 4096)

            # get inspect lkm
            fcntl.ioctl(device, self.cmd_get_inspect_lkm, buf)
            lkm_path = buf.tostring()
            lkm_path = lkm_path.replace('\x00', '')

            # inspect lkm_path
            result = inspect(lkm_path, '/var/log/lkrd_db.dat')
            result_str = "[" + lkm_path + "] result : " + str(result)   
            logger.info(result_str)

            # send inspect result
            fcntl.ioctl(device, self.cmd_send_inspect_result, struct.pack('L', result))
            time.sleep(1)

            device.close()

if __name__ == "__main__":
    daemon = lkrd_daemon()
    logger = logging.getLogger("daemon_log")
    logger.setLevel(logging.INFO)
    formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
    handler = logging.FileHandler("/var/log/lkrd_daemon.log")
    handler.setFormatter(formatter)
    logger.addHandler(handler)

    daemon_runner = runner.DaemonRunner(daemon)

    #This ensures that the logger file handle does not get closed during daemonization
    daemon_runner.daemon_context.files_preserve=[handler.stream]
    daemon_runner.do_action()

