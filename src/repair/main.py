import argparse
import os
from os.path import join, exists, relpath, basename, realpath
import shutil
import stat
import statistics
import time
import logging

from transformation import VulnerableTransformer, FixInjector
from project import Frontend, Backend, Validation
from testing import Tester
from inference import Inferrer
from synthesis import Synthesizer

from utils import format_time, time_limit, TimeoutException

logging.basicConfig(filename="main.log", level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
logger = logging.getLogger(__name__)


SYNTHESIS_LEVELS = ['alternatives',
                    'integer-constants',
                    'boolean-constants',
                    'variables',
                    'basic-arithmetic',
                    'basic-logic',
                    'basic-inequalities',
                    'extended-arithmetic',
                    'extended-logic',
                    'extended-inequalities',
                    'mixed-conditional',
                    'conditional-arithmetic']


class DG:
    def __init__(self, working_dir, src, buggy, oracle, tests, vulnerable, build, configure, config):
        self.working_dir = working_dir
        self.config = config
        self.test_suite = tests[:]

        extracted = join(working_dir, 'extracted')
        os.mkdir(extracted)

        angelic_forest_file = join(working_dir, 'last-angelic-forest.json')
        

        self.instrument_for_inference = VulnerableTransformer(vulnerable, config, extracted)
        self.tester = Tester(oracle, config, working_dir)
        self.infer_spec = Inferrer(config, self.tester)
        self.synthesize_fix = Synthesizer(config, extracted, angelic_forest_file, working_dir)
        self.apply_patch = FixInjector(config)

        validation_dir = join(working_dir, "validation")
        shutil.copytree(src, validation_dir, symlinks=True)
        self.validation_src = Validation(config, validation_dir, buggy, build, configure)
        self.validation_src.configure()
        compilation_db = self.validation_src.export_compilation_db()
        self.validation_src.import_compilation_db(compilation_db)
        #self.validation_src.initialize()
        
        frontend_dir = join(working_dir, "frontend")
        shutil.copytree(src, frontend_dir, symlinks=True)
        self.frontend_src = Frontend(config, frontend_dir, buggy, build, configure)
        self.frontend_src.import_compilation_db(compilation_db)
        #self.frontend_src.initialize()
		self.frontend_src.build()

        backend_dir = join(working_dir, "backend")
        shutil.copytree(src, backend_dir, symlinks=True)
        self.backend_src = Backend(config, backend_dir, buggy, build, configure)    
        self.backend_src.import_compilation_db(compilation_db)
        self.backend_src.initialize()
        self.backend_src.build() 




    def generate_patch(self):
        self.instrument_for_inference(self.backend_src)
        self.backend_src.configure()
        self.backend_src.build()
        
        angelic_forest = {}
        for test in self.test_suite:
            angelic_path = self.infer_spec(self.backend_src, test)
            angelic_forest[test] = angelic_path
        logger.info(angelic_forest)
        initial_fix = self.synthesize_fix(angelic_forest)
        self.apply_patch(self.validation_src, initial_fix)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    
    parser.add_argument('--src', metavar='SOURCE', help='source directory')
    parser.add_argument('--buggy', metavar='BUGGY', help='relative path to buggy file')
    parser.add_argument('--record', metavar='RECORD', help='relative path to record file')
    parser.add_argument('--oracle', metavar='ORACLE', help='oracle script')
    parser.add_argument('--tests', metavar='TEST', nargs='+', help='test case')
    parser.add_argument('--vulnerable', help='file specifying addresses of vulnerable statements')
    parser.add_argument('--entry', help='file specifying addresses of entry statements')
    parser.add_argument('--build', default='make -e') # -e with modified environment
    parser.add_argument('--configure', default=None)
    parser.add_argument('--verbose', default=True)
    parser.add_argument('--mute_build_message', default=True)
    parser.add_argument('--mute_warning', default=True)
    parser.add_argument('--synthesis-levels', metavar='LEVEL',\
                        nargs='+', choices=SYNTHESIS_LEVELS,\
                        default=['alternatives', 'integer-constants', 'boolean-constants'])
    parser.add_argument('--synthesis-timeout', metavar='MS', type=int, default=30000, 
                        help='synthesis timeout (default: %(default)s)') # 30 secs

    args = parser.parse_args()


    
    def rm_force(action, name, exc):
        os.chmod(name, stat.S_IREAD)
        shutil.rmtree(name)
    working_dir = join(os.getcwd(), ".dg")
    if exists(working_dir):
        shutil.rmtree(working_dir, onerror=rm_force)
    os.mkdir(working_dir)
    logger.info("working_dir {}".format(working_dir))


    start = time.time()
    
    tool = DG(working_dir, args.src, args.buggy, args.oracle, args.tests, args.vulnerable, args.build, args.configure, args)
    tool.generate_patch()
    


    end = time.time()
    elapsed = format_time(end - start)

    logger.info("patch generated in {}".format(elapsed))
