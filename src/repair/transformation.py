import subprocess
import sys
from os.path import join, exists, relpath, basename, realpath
import os
from utils import cd
import logging

logger = logging.getLogger(__name__)


class VulnerableTransformer():
    def __init__(self, vulnerable, config):
        self.vulnerable = vulnerable
        self.config = config
        if self.config.verbose:
            self.subproc_output = sys.stderr
        else:
            self.subproc_output = subprocess.DEVNULL
        

    def __call__(self, project):
        src = basename(project.dir)
        logger.info('instrumenting suspicious of {} source'.format(src))
        environment = os.environ
        environment['ANGELIX_GLOBAL_VARIABLES'] = 'YES'
        environment['ANGELIX_FUCTION_PARAMENTERS'] = 'YES'
        
        environment['ANGELIX_SUSPICIOUS'] = self.vulnerable
        
        with cd(project.dir):
            subprocess.call(['instrument-suspicious', project.buggy],
                        stderr=self.subproc_output,
                        stdout=self.subproc_output,
                        env=environment)
