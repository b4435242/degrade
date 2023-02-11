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

    def __init__(self, oracle, config, workdir):
        self.config = config
        self.oracle = oracle
        self.workdir = workdir

    def __call__(self, project, test, klee=False, env=os.environ):
        environment = env

        if self.config.verbose:
            subproc_output = sys.stderr
        else:
            subproc_output = subprocess.DEVNULL

        if klee:
            environment['ANGELIX_RUN'] = 'angelix-run-klee'
        
        with cd(project.dir):
             proc = subprocess.call(self.oracle+" "+test,
                                    env=environment,
                                    stdout=subproc_output,
                                    stderr=subproc_output,
                                    shell=True)

        logger.info('klee complete')

    def get_target(self, project):
        with open(join(project.dir, 'target'), 'r') as f:
            target = f.read()
        return target
