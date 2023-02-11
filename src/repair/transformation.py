import subprocess
import sys
from os.path import join, exists, relpath, basename, realpath
import os
import tempfile
from utils import cd
import logging
import re
import shutil
logger = logging.getLogger(__name__)


class RecordTransformer():
    def __init__(self, locations, config):
        self.locations = locations
        self.config = config
        if self.config.verbose:
            self.subproc_output = sys.stderr
        else:
            self.subproc_output = subprocess.DEVNULL
        

    def __call__(self, project):
        src = basename(project.dir)
        logger.info('instrumenting record of {} source'.format(src))
        environment = os.environ



        with cd(project.dir):
            argv = ['record-snapshot', project.buggy] # Actually recording file here
            argv += self.read_locations()
            logger.info('argv {}'.format(argv))
            subprocess.call(argv,
                        stderr=self.subproc_output,
                        stdout=self.subproc_output,
                        env=environment)
    
    def read_locations(self):
        with open(self.locations, 'r') as f:
            locs = re.split('\s', f.read())
        return locs
      


class VulnerableTransformer():
    def __init__(self, vulnerable, config, extracted):
        self.vulnerable = vulnerable
        self.config = config
        self.extracted = extracted
        if self.config.verbose:
            self.subproc_output = sys.stderr
        else:
            self.subproc_output = subprocess.DEVNULL
        

    def __call__(self, project):
        src = basename(project.dir)
        logger.info('instrumenting suspicious of {} source'.format(src))
        environment = os.environ
        environment['ANGELIX_EXTRACTED'] = self.extracted



        with cd(project.dir):
            if project.buggy==project.record:
                argv = ['instrument-suspicious', project.buggy]
                argv += (self.read_locations(self.vulnerable) + self.read_locations(self.entry))
                logger.info('argv {}'.format(argv))
                subprocess.call(argv,
                        stderr=self.subproc_output,
                        stdout=self.subproc_output,
                        env=environment)
            else:
                argv = ['instrument-suspicious', project.buggy]
                argv += self.read_locations(self.vulnerable)
                logger.info('argv {}'.format(argv))
                subprocess.call(argv,
                        stderr=self.subproc_output,
                        stdout=self.subproc_output,
                        env=environment)
                argv = ['instrument-suspicious', project.record]
                argv += self.read_locations(self.entry)
                logger.info('argv {}'.format(argv))
                subprocess.call(argv,
                        stderr=self.subproc_output,
                        stdout=self.subproc_output,
                        env=environment)


    def read_locations(self, path):
        with open(path, 'r') as f:
            locs = re.split('\s', f.read())
        return locs
               

class FixInjector:

    def __init__(self, config):
        self.config = config
        if self.config.verbose:
            self.subproc_output = sys.stderr
        else:
            self.subproc_output = subprocess.DEVNULL

    def __call__(self, project, patch):
        src = basename(project.dir)
        logger.info('applying patch to {} source'.format(src))

        environment = dict(os.environ)
        dirpath = tempfile.mkdtemp()
        patch_file = join(dirpath, 'patch')
        with open(patch_file, 'w') as file:
            for e, p in patch.items():
                file.write('{} {} {} {}\n'.format(*e))
                file.write(p + "\n")


        environment['ANGELIX_PATCH'] = patch_file

        with cd(project.dir):
            return_code = subprocess.call(['apply-patch', project.buggy],
                                          stderr=self.subproc_output,
                                          stdout=self.subproc_output,
                                          env=environment)
        if return_code != 0:
            if self.config['ignore_trans_errors']:
                logger.error("transformation of {} failed".format(relpath(project.dir)))
            else:
                logger.error("transformation of {} failed".format(relpath(project.dir)))
                raise TransformationError()

        shutil.rmtree(dirpath)

        pass
