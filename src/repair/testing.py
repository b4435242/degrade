import os
from os.path import basename, join, exists
from utils import cd
import subprocess
import logging
import sys
import tempfile
from glob import glob

logger = logging.getLogger(__name__)

class Tester:

    def __init__(self, config, workdir):
        self.config = config
        #self.oracle = oracle
        self.workdir = workdir

    def __call__(self, project, klee=False, env=os.environ):
        environment = env

        if self.config.verbose:
            subproc_output = sys.stderr
        else:
            subproc_output = subprocess.DEVNULL

        if klee:
            klee_cmd = 'angelix-run-klee'
        target = self.get_target(project)
        cmd = '%s %s' % (klee_cmd, target)
        with cd(project.dir):
             proc = subprocess.call(cmd,
                                    env=environment,
                                    stdout=subproc_output,
                                    stderr=subproc_output,
                                    shell=True)

        logger.info('klee complete')

    def get_target(self, project):
        with open(join(project.dir, 'target'), 'r') as f:
            target = f.read()
        return target
