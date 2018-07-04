'''
Created on 21.03.2016

@author: hdh
'''

import sys
import os
import shutil
import socket
import subprocess
from   types import *

import argparse
import ConfigParser

import qa_util
from qa_init import ext_tables_dialog

class QaConfig(object):
    '''
    classdocs
    '''

    def __init__(self, qa_src, argv=[]):
        '''
        Constructor
        '''
        if len(argv):
            self.argv=argv
        else:
            self.argv=sys.argv[1:]

        self.qa_src = qa_src

        # dictionary for the final result

        self.dOpts={}

        self.project=''
        self.project_as=''

        self.lSelect=[]
        self.lLock=[]

        self.prjs_avail = [ 'CORDEX', 'CMIP5', 'CMIP6', 'HAPPI' ]

        # a list of dicts, first for the command-line options,
        # then those from successive QA_CONF files.
        self.ldOpts=[]

        if len(qa_src):
            self.ldOpts.append({})
            self.ldOpts[0]['QA_SRC']=qa_src

        self.cfg = CfgFile(self)

        self.run()

        self.ldOpts=[] # free mem
        self.lSelect=[]
        self.lLock=[]

        self.addOpt("CURR_DIR", self.curr_dir)


    def addOpt(self, key, val, dct={}):
        if len(dct):
            local_dct = dct
        else:
            local_dct = self.dOpts

        local_dct[key]=val

        return


    def backward_compat(self):
        # rather intricate, but ensures later correct merging of options
        for ldo in self.ldOpts:
            if len(ldo):
                if self.isOpt('EXP_NAME', dct=ldo):
                    self.addOpt('LOG_FNAME', ldo['EXP_NAME'], dct=ldo)
                    del self.dOpts['EXP_NAME']

                if self.isOpt('EXP_FNAME_PATTERN', dct=ldo):
                    self.addOpt('LOG_FNAME_PATTERN', ldo['EXP_FNAME_PATTERN'],
                                dct=ldo)
                    del ldo['EXP_FNAME_INDEX']

                if self.isOpt('EXP_PATH_BASE', dct=ldo):
                    self.addOpt('DRS_PATH_BASE', ldo['EXP_PATH_BASE'], dct=ldo)
                    del ldo['EXP_PATH_BASE']

                if self.isOpt('EXP_PATH_INDEX', dct=ldo):
                    self.addOpt('LOG_PATH_INDEX', ldo['EXP_PATH_INDEX'],
                                dct=ldo)
                    del ldo['EXP_PATH_INDEX']

                if self.isOpt('SHOW_EXPS', dct=ldo):
                    self.addOpt('SHOW_EXP', ldo['SHOW_EXPS'], dct=ldo)
                    del ldo['SHOW_EXPS']

                if self.isOpt('PT_PATH_INDEX', dct=ldo):
                    self.addOpt('CT_PATH_INDEX', ldo['PT_PATH_INDEX'], dct=ldo)
                    del ldo['PT_PATH_INDEX']

                if self.isOpt('SUMMARY_ONLY', dct=ldo):
                    self.addOpt('ONLY_SUMMARY', ldo['SUMMARY_ONLY'], dct=ldo)
                    del ldo['SUMMARY_ONLY']

                if self.isOpt('SUMMARY', dct=ldo):
                    self.addOpt('ONLY_SUMMARY', ldo['SUMMARY'], dct=ldo)
                    del ldo['SUMMARY']

        return


    def commandLineOpts(self, parser):
        # Some conversions of command-line parameters; just to be backward
        # compatible
        llArgv = self.argv
        lArgv=[]

        for i in range(len(llArgv)):
            str0=llArgv[i]

            if str0[0:7] == 'install':
                str0 = '--' + str0
            elif str0[0:2] == '-e' or str0[0:2] == '-E':
                # conversion to '-eabc=xyz'
                if str0 == '-e' or str0 == '-E':
                   # indicates the next one
                   llArgv[i+1]='-e' + llArgv[i+1]
                   continue
                elif str0[0:3] == '-e_' or str0[0:3] == '-E_':
                    str0='-e' + str0[3:]
                elif str0[0:2] == '-E':
                    str0='-e' + str0[2:]

            # convert long-option key-words to lower-case,
            # e.g. --ABC=qWeR --> --abc=qWeR
            elif str0[0:2] == '--' :
               pos = str0.find('=')
               if not str0[:pos].islower():
                  if pos == -1:
                     str0=str0.lower()
                  else:
                     str0=str0[:pos].lower() + str0[pos:]

            lArgv.append(str0)

        if len(lArgv) == 0: # calling without any parameter raises help
            lArgv.append('-h')

        # let's parse
        args = parser.parse_args(lArgv)

        # post-processing: namespace --> dict
        _ldo={}

        # apply for the list provided by -e_
        if args.eTypeOpt != None:
            for i in range(len(args.eTypeOpt)):
                # convert the key-words to upper-case,
                pos = args.eTypeOpt[i].find('=')
                if pos == -1:
                    key=args.eTypeOpt[i].upper()
                    val=True
                else:
                    key=args.eTypeOpt[i][:pos].upper()
                    val=args.eTypeOpt[i][pos+1:]
                    if val == 't':
                        val = True
                    elif val == 'f':
                        val = False

                # special
                if key == 'NEXT' and val == True:
                    args.NEXT = 1
                    val = 1
                if key == 'NEXT_VAR' and val == True:
                    args.NEXT_VAR = 1
                    val = 1

                if key == 'QA_TABLES':
                    self.qa_tables=val
                if key == 'QA_HOME':
                    self.qa_tables=val

                # convert comma-sep val to list

                # comma-sep list --> python list, but protect
                # those containing braces
                if type(val) == StringType:
                    if not '{' in val:
                        try:
                            # --> list of integers
                            val=[ int(x) for x in val.split(',') ]
                            if len(val) == 1:
                                val = val[0]
                        except ValueError:
                            # --> list of strings
                            s=repr(val)
                            if ',' in s:
                                val=val.split(',')

                            if len(val) == 1:
                                val = val[0]
                        except:
                            pass

                _ldo[key] = val

        # special: long-opts
        if args.PROJECT     != None:
            _ldo['PROJECT']    = args.PROJECT
            self.project = args.PROJECT
        if args.PROJECT_AS  != None:
            _ldo['PROJECT_AS'] = args.PROJECT_AS
            self.project_as = args.PROJECT_AS

        if args.SHOW_NEXT > 0:
            _ldo['NEXT']      = args.SHOW_NEXT
            args.SHOW_CALL = True

        if args.AUTO_UP     != None: _ldo['AUTO_UP']      = args.AUTO_UP
        if args.CONFIG_FILE != None: _ldo['CONFIG_FILE']  = args.CONFIG_FILE
        if args.QA_TABLES   != None: _ldo['QA_TABLES']    = args.QA_TABLES
        if args.TASK        != None: _ldo['TASK']         = args.TASK
        if args.CMD_UPDATE  != None: _ldo['CMD_UPDATE']   = args.CMD_UPDATE

        if args.DRYRUN:              _ldo['DRY_RUN']        = True
        if args.NEXT  > 0:
            _ldo['NEXT']           = args.NEXT
            if args.NEXT_VAR == 0:
                _ldo['NEXT_VAR']   = 1
        if args.NEXT_VAR > 0:        _ldo['NEXT_VAR']       = args.NEXT_VAR
        if args.QA_EXAMPLE:          _ldo['QA_EXAMPLE']     = args.QA_EXAMPLE

        if args.SHOW_CALL:           _ldo['SHOW_CALL']      = args.SHOW_CALL
        if args.SHOW_CONF:           _ldo['SHOW_CONF']      = args.SHOW_CONF
        if args.SHOW_EXP:            _ldo['SHOW_EXP']       = args.SHOW_EXP
        if args.SHOW_VERSION:        _ldo['SHOW_VERSION']   = True
        if args.STATUS_LINE:
            _ldo['STATUS_LINE'] = args.STATUS_LINE
            _ldo['STATUS_LINE_SZ'] = 0

        if args.WORK:               _ldo['WORK']            = args.WORK

        # collect for passing to QA-DKRZ/install.

        # plain nc-files and/or something like install="arg1 arg2=asdf,arg3"
        # at first, decompose items of install and then, items themselves.
        inst_args=[]
        for i in range(len(args.NC_FILE)):
            if i >= len(args.NC_FILE):
                continue

            arg = args.NC_FILE[i]

            if arg[0:7] == 'install':
                if len(arg) == 7:
                    x = arg[8:].split(' ')
                else:
                    x=[]

                x_s=[]
                for y in x:
                    x_s.extend(y.split(','))

                for x in x_s:
                    if x[0:2] == '--':
                        x=x[2:]

                    if x.upper() in self.prjs_avail:
                        # note: multiple assignments possible for install, but
                        # only a single one for operation.
                        qa_util.add_unique(x.upper(), inst_args)
                        _ldo['PROJECT'] = arg.upper()
                    else:
                        qa_util.add_unique(x, inst_args)

                    if 'ship' in x and not 'unship' in x:
                        qa_util.add_unique('force', inst_args)
            else:
                # this allows for something like 'install up PROJECTS'
                # Note that --arg is caught elsewhere by the parser
                if arg.upper() in self.prjs_avail:
                    # note: multiple assignments possible for install, but
                    # only a single one for operation.
                    _ldo['PROJECT'] = arg.upper()
                else:
                    qa_util.add_unique(arg, inst_args)

            if arg[-3:] != '.nc':
                del args.NC_FILE[i]

        # special: SELECT | LOCK
        if len(args.NC_FILE) == 0:
            if args.CL_L != None:
                s= 'LOCK ' + args.CL_L
                self.lLock.append(self.setSelLock(s.replace(' ','') ) )
            if args.CL_S != None:
                s= 'SELECT ' + args.CL_S
                self.lSelect.append(self.setSelLock(s.replace(' ','') ) )
        else:
            self.lSelect.append(args.NC_FILE[0])

        # regular --install='args'
        if args.INSTALL != None:
            for inst in args.INSTALL:
                l_0 = inst.split(',')
                l_1=[]
                for l in l_0:
                    l_1.extend(l.split(' '))

                for val in l_1:
                    if val == '--':
                        val=val[2:]

                    qa_util.add_unique(val, inst_args)

        if args.QA_TABLES != None:
            qa_util.add_unique('qa_tables=' + args.QA_TABLES, inst_args)

        if args.AUTO_UP != None:
            qa_util.add_unique('auto-up', inst_args)

        if args.FORCE:
            qa_util.add_unique('force', inst_args)

        if args.FREEZE:
            qa_util.add_unique('freeze', inst_args)

        if args.SHIP != None and len(args.SHIP):
            qa_util.add_unique('ship=' + args.SHIP, inst_args)
            qa_util.add_unique('force', inst_args)

        if args.UNFREEZE:
            qa_util.add_unique('unfreeze', inst_args)

        if args.UNSHIP:
            qa_util.add_unique('unship', inst_args)

        if inst_args:
            str0=''
            for s in inst_args:
                str0 += s + ' '

            _ldo['INSTALL']=str0

        return _ldo


    def copyDefaultTables(self):
        '''
        1. The user should have write-access to tables.
        2. The tables in QA_SRC may be write-protected (e.g. installed by admins)
        3. User applications will wget external tables.
        Solution: The tables are copied to another location writable by the user.
                Users are asked for a path stored in QA_TABLES during 'install'
        '''

        # rsync the default tables
        src=os.path.join(self.qa_src, 'tables')

        if os.path.isdir(src):
            dest=self.getOpt('QA_TABLES')

            if not dest:
               dest=os.path.join(self.home, 'tables')

            rsync_cmd='rsync' + ' -auz' + ' --exclude=*~ ' + src + ' ' + dest

            try:
               subprocess.call(rsync_cmd, shell=True)
               #shutil.copytree(src, dest)
            except:
               print 'could not rsync ' + src + ' --> ' + dest
               sys.exit(1)

        return


    def create_parser(self):
        parser = argparse.ArgumentParser(
            prog="qa-dkrz",
            description="Meta-data checker QA-DKRZ of climate data NetCDF files."
            )

        parser.add_argument( '--auto-up', '--auto_up','--auto-update',
            '--auto_update', '--auto',
            nargs='?', const='enable', dest='AUTO_UP',
            help="Passed to install")

        parser.add_argument('--config-file', dest='CONFIG_FILE',
            help="Contains paths and update frequencies.")

        parser.add_argument('--dry', '--dry-run', dest='DRYRUN',
            action="store_true",
            help='Show only filenames which are to be processed')

        parser.add_argument( '--example',
            action="store_true", dest='QA_EXAMPLE',
            help="Run the example.")

        parser.add_argument( '-e', '-E',
            action='append', dest='eTypeOpt',
            help="QA configuration: [-e|-E]key[=value].")

        parser.add_argument('-f', '--task', dest='TASK',
            help="QA task-file with frequently changing options.")

        parser.add_argument('--force', action="store_true", dest='FORCE',
            help="Force QA install update.")

        parser.add_argument('--freeze', action="store_true", dest='FREEZE',
            help="Freeze QA, externals and tables.")

        parser.add_argument('--install', nargs='*', dest='INSTALL',
            help="Comma-sep-list passed to the install script.")

        parser.add_argument('-L', '--lock', dest='CL_L',
            help="Exclude from checking, syntax see LOCK in QA configure file.")

        parser.add_argument('-m', dest='STATUS_LINE',
            action="store_true",
            help="display: next atomic variable (#done/#all).")

        parser.add_argument('-n', '--next', dest='NEXT',
            type=int, nargs='?', default=0, const=1,
            help="Proceed for a limited number of variables, unlimited by default.")

        parser.add_argument('-nv', '--next_variable', dest='NEXT_VAR',
            type=int, nargs='?', default=0, const=1,
            help="Proceed for a limited number of variables, unlimited by default.")

        parser.add_argument('-P', '--project', dest='PROJECT',
            help= "PROJECT name.")

        parser.add_argument('--project_as', '--project-as', dest = 'PROJECT_AS',
            help= "Use assigned project for processing but PROJECT name is kept.")

        parser.add_argument( '--qa_tables', '--QA_TABLES', '--qa_home', '--QA_HOME',
            dest='QA_TABLES',
            help='''Path to tables, HOME/.qa-dkrz/tables by default.''')

        parser.add_argument('-S', '--select', dest='CL_S',
            help='''Selection of variables; overrules any SELECT assignment.''')

        parser.add_argument('--ship', dest='SHIP',
            help='''Path to store QA-DKRZ for ship.''')

        parser.add_argument('--show_call', '--show-call', dest='SHOW_CALL',
            action="store_true",
            help='''Show C++ executable call; number depends on --next''')

        parser.add_argument('--show_conf', '--show-conf', dest='SHOW_CONF',
            action="store_true",
            help="Show the configuration of the current run")

        parser.add_argument('--show_exp', '--show-exps',
            '--SHOW_EXP', '--SHOW_EXPS', dest='SHOW_EXP', action="store_true",
            help='''Show selected experiments and paths.''')

        parser.add_argument('--show_next', '--show-next',
            type=int, nargs='?', default=0, const=1, dest='SHOW_NEXT',
            help="Show the N next path/file for executaion [N=1].")

        parser.add_argument('--unfreeze', action="store_true", dest='UNFREEZE',
            help="Unfreeze QA.")

        parser.add_argument('--unship', dest='UNSHIP',
            action="store_true",
            help='''Initialisation after shippingi at path.''')

        parser.add_argument('--up', '--update', dest='CMD_UPDATE',
            nargs='?',  const='freeze',
            help='auto | [freeze] | daily : Run with qa-dkrz install.')

        parser.add_argument('--version',
            action="store_true", dest='SHOW_VERSION',
            help='Display version information of QA_DKRZ and tables.')

        parser.add_argument( '--work', dest='WORK',
            help='''Only used for --example.''')

        parser.add_argument('NC_FILE', nargs='*',
            help= "NetCDF files [file1.nc[, file2.nc[, ...]]].")

        return parser


    def delOpt(self, key, dct={}):
        if len(dct):
            local_dct = dct
        else:
            local_dct = self.dOpts

        if key in local_dct:
            del local_dct[key]

        return


    def finalSelLoc(self):
        p=[]
        v=[]
        self.dOpts['SELECT_PATH_LIST'] = []
        self.dOpts['SELECT_VAR_LIST']  = []

        if not self.isOpt("PROJECT_DATA"):
            if len(self.lSelect):
                h,t = os.path.split(self.lSelect[0])
                if not (h or t):
                    print 'missing NetCDF file or directory'
                    sys.exit(1)

                #if h:
                #    h1,h2 = os.path.split(h)
                #    self.dOpts["PROJECT_DATA"] = h1

                x_t = t.split('_')
                str0=''
                if h:
                    str0 = h + '='
                if t:
                    str0 += x_t[0] + '_'

                self.lSelect[0]=str0
                self.dOpts["NEXT"] = 1

        for sel in self.lSelect:
            (p,v)=self.getSelLock('S', sel)

            for ix in range(len(p)):
                self.dOpts['SELECT_PATH_LIST'].append(p[ix])
                self.dOpts['SELECT_VAR_LIST'].append(v[ix])

        p=[]
        v=[]
        self.dOpts['LOCK_PATH_LIST'] = []
        self.dOpts['LOCK_VAR_LIST']  = []

        for loc in self.lLock:
            (p,v)=self.getSelLock('L', loc)

            for ix in range(len(p)):
                self.dOpts['LOCK_PATH_LIST'].append(p[ix])
                self.dOpts['LOCK_VAR_LIST'].append(v[ix])

        return

    def getCFG_opts(self, qa_src):
        # read the config file; may also convert an old plain text file;
        # cfg assignments are also appended to qaConf
        #self.ldOpts.append( self.cfg.getOpts() )

        return self.cfg.getOpts()


    def getOpt(self, key, dct={}, bStr=False):
        if len(dct):
            curr_dct = dct
        else:
            curr_dct = self.dOpts

        val=''
        if key in curr_dct:
            val = curr_dct[key]

            if bStr:
               val = str(val)

        return val


    def get_qa_tables(self):
        qtc = []  # QA_TABLES candidates

        sects = self.cfg.rCP.sections()
        qt="QA_TABLES"

        for sect in sects:
            if self.cfg.rCP.has_option(sect, qt):
                qtc.append( self.cfg.rCP.get(sect, qt) )

        return qtc


    def getSelLock(self, key, line):
        # Note for SELECT, LOCK, -S and -L usage:
        # General setting: NAME {space | = | +=}[path[=]][variable]
        #                  already resolved

        # Examples:
        # 1) path=var
        #    specifies a single path where to look for a single variable
        # 2) p1,p2=var
        #    two paths to look for a variable
        # 3) p1,p2=
        #    two paths with every variable, equivalent to p1,p2=.*
        # 4) p1=v1,v2
        #    one path with two variables
        # 5) str1[,str2,str3]
        #    a) no '/' --> variables
        #    b) '/' anywhwere  --> only paths; no trapping of errors
        # 6) relative path without a '/' must have '=' appended
        # 7) p1=,p2=v1...  --> ERROR

        # special: a fully qualified file
        if os.path.isfile(line):
            h, t=os.path.split(line)
            if h and t:
                line = h + '=' + t
            elif len(h):
                line = h+'='
            else:
                line = t

        if line[0] == '=':
            line = line[1:]
        elif line[0:2] == '+=':
            line = line[2:]

        x_line=line.split('=')

        sz=len(x_line)
        if sz > 2 or sz == 0:
            return # input failure
        elif sz > 1:
            p=x_line[0]
            if len(x_line[1]):
                v=x_line[1]
            else:
                v='.*'
        elif sz == 1:
            if '/' in x_line[0]:
                # no variable
                v='.*'
                p=x_line[0]
            else:
                # no path
                p='.*'
                v=x_line[0]

        # comma-separated items?
        x_p=p.split(',')
        x_v=v.split(',')

        lp=[]
        lv=[]
        for p in x_p:
            for v in x_v:
                lp.append(p)
                lv.append(v)

        return ( lp, lv)

    def isOpt(self, key, dct={}):
        # a) an option exists, then return True, but,
        # b) if the type is bool, then return the value
        if len(dct):
            curr_dct = dct
        else:
            curr_dct = self.dOpts

        if key in curr_dct:
            val = curr_dct[key]

            if type(val) == BooleanType:
                return val
            else:
                if not val == 'f':
                    return True

        return False


    def mergeOptions(self):
        # merge reversed, i.e. from low to high priority
        for ix in range(len(self.ldOpts)-1,-1,-1):
            if len(self.ldOpts[ix]):
                for key in self.ldOpts[ix]:
                    if key == 'QA_CONF' and key in self.dOpts:
                        self.dOpts[key] += ',' + self.ldOpts[ix][key]

                    elif key == 'SELECT' and key in self.dOpts:
                        if len(self.dOpts[key]):
                            self.dOpts[key] += ','

                        # SELECT [+=] str: accumulates, SELECT = str: overrules
                        if self.ldOpts[ix][key][0:2] == '+=':
                            self.dOpts[key] += self.ldOpts[ix][key][2:]
                        elif self.ldOpts[ix][key][0] == '=':
                            self.dOpts[key] = self.ldOpts[ix][key][1:]

                    else: # overwrite
                        self.dOpts[key] = self.ldOpts[ix][key]


        return

    def readQA_CONF_files(self):

        # list config files, if any.
        lCfgFiles=[]

        # At first, any item specified by QA_CONF on the command-line,
        # which is given by index 1
        _ldo = self.ldOpts[1]

        if 'QA_CONF' in _ldo:
            lCfgFiles.append(_ldo['QA_CONF'])

        if 'TASK' in _ldo:
            lCfgFiles.append(_ldo['TASK'])

        if len(lCfgFiles) == 0:
            lCfgFiles.append('') # force to enter the for-loop

        for cfg in lCfgFiles:
            self.ldOpts.append({})
            _ldo = self.ldOpts[len(self.ldOpts)-1]

            if len(cfg):
                if not os.path.isfile(cfg):
                    print 'no such file '+cfg
                    sys.exit(1)

                with open(cfg, 'r') as f:
                    select=''
                    lock=''
                    for line in f:
                        line = qa_util.clear_line(line)
                        if len(line) == 0:
                            continue

                        # SELECT|LOCK may have '=', '+=', or ' '; the latter
                        # two accumulate and the value may have embedded '='
                        if line[:6] == 'SELECT':
                            str0=self.setSelLock(line)
                            if str0[0] == '=':
                                select = str0[1:]
                            else:
                                if len(select):
                                    select += ','
                                select += str0[2:]
                            continue
                        elif line[:4] == 'LOCK':
                            str0=self.setSelLock(line)
                            if str0[0] == '=':
                                lock = str0[1:]
                            else:
                                if len(lock):
                                    lock += ','
                                lock += str0[2:]
                            continue
                        else:
                            pos = line.find('=')
                            if pos == -1:
                                key=line.upper()
                                val='t'
                                if key == 'NEXT' or key == 'NEXT_VAR':
                                    val=1
                                elif key == 'SHOW_NEXT':
                                    val=1
                            else:
                                (key, val) = line.split('=', 1)
                                key=key.upper()

                        if key == 'PROJECT' and len(self.project) == 0:
                            # self.project is set to give precedence to 1st key
                            self.project = val

                        elif key == 'PROJECT_AS' and len(self.project_as) == 0:
                            self.project_as = val

                        elif len(key) > 2 and key[0:3] == 'QC_':
                            # --> backward compatible
                            key = 'QA' + key[2:]

                        # comma-sep list --> python list, but protect
                        # those containing braces
                        if not '{' in val:
                            try:
                                # --> list of integers
                                val=[ int(x) for x in val.split(',') ]

                                # but, only a single one
                                if len(val) == 1:
                                   val = val[0]
                            except ValueError:
                            # --> list of strings
                                s=repr(val)
                                if ',' in s:
                                    val=val.split(',')
                            except:
                                pass

                        _ldo[key] = val  # fill in regular QA options

                    if len(select):
                        self.lSelect.extend(select.split(','))
                    if len(lock):
                        self.lLock.extend(lock.split(','))

            if 'QA_CONF' in _ldo:
                str0 = _ldo['QA_CONF']
                del _ldo['QA_CONF']
            else:
                str0='' # default configuration

            self.searchQA_CONF_chain(str0, lCfgFiles)

        # special
        if 'NEXT' in _ldo:
            if not 'NEXT_VAR' in _ldo:
                _ldo['NEXT_VAR'] = 1

        # preserve the list of configuration files
        if len(self.ldOpts):
            val=''
            for cfg in lCfgFiles:
                if len(cfg):
                    if len(val):
                        val += ','
                    val += cfg

            self.ldOpts[0]['QA_CONF'] = val

        return


    def run(self):
        cLO = self.commandLineOpts( self.create_parser() )

        self.curr_dir = os.getcwd()

        # is it defined in QA_SRC/.qa-conf ? Else: try home
        self.home = os.path.join( os.environ['HOME'], '.qa-dkrz')

        if os.path.isfile( os.path.join(self.qa_src, '.qa-config.txt') ):
           self.cfg_file=os.path.join(self.qa_src, '.qa-config.txt')
        elif os.path.isfile( os.path.join(self.qa_src, '.qa.cfg') ):
           self.cfg_file=os.path.join(self.qa_src, '.qa.cfg')
        elif os.path.isfile( os.path.join(self.home, 'config.txt') ):
           self.cfg_file=os.path.join(self.home, 'config.txt')
        else:
           self.cfg_file=os.path.join(self.home, 'qa.cfg')

        self.cfg.read_file( self.cfg_file, section=self.qa_src)

        self.cfg_opts = self.getCFG_opts(self.qa_src) # (multiple) ..._qa.conf files

        # concatenate dictionaries with precedence: high --> low
        if len(cLO):
            self.ldOpts.append(cLO)

        if len(self.cfg_opts):
            self.ldOpts.append(self.cfg_opts)  # config file in HOME/.qa-conf
        else:
            self.ldOpts[-1]

        if self.isOpt("QA_TABLES", dct=self.cfg_opts):
            self.getOpt("QA_TABLES", dct=self.cfg_opts)
            self.readQA_CONF_files()

        self.setDefault()

        self.backward_compat()
        self.mergeOptions()

        self.finalSelLoc()

        # backward compatibility
        if "AUTO_UP" in self.dOpts or "AUTO_UPDATE" in self.dOpts:
            self.dOpts["CMD_UPDATE"] = "auto"

        '''
        if not "DEFAULT_PROJECT" in self.dOpts :
            self.dOpts['DEFAULT_PROJECT'] = 'CORDEX'
            if not 'INSTALL' in self.dOpts:
                self.dOpts['INSTALL'] = 'set-default-project=CORDEX'
            else:
                self.dOpts['INSTALL'] += ' set-default-project=CORDEX'
        '''

        # modify definitions if P_AS is set: P_AS --> P and P --> P_DRVD
        if 'PROJECT_AS' in self.dOpts:
            if 'PROJECT' in self.dOpts:
                # sequence matters
                self.addOpt('PROJECT_VIRT', self.dOpts['PROJECT'])
                self.addOpt('PROJECT', self.dOpts['PROJECT_AS'])
            else:
                self.addOpt('PROJECT_VIRT', self.dOpts['PROJECT_AS'])
                self.addOpt('PROJECT', self.dOpts['PROJECT_AS'])

            self.project_virt = self.dOpts['PROJECT_VIRT']
            self.delOpt('PROJECT_AS')

        #if not self.isOpt('PROJECT') and self.isOpt('DEFAULT_PROJECT'):
        #    self.addOpt('PROJECT', self.dOpts['DEFAULT_PROJECT'])

        self.project = self.getOpt('PROJECT')

        if not self.isOpt("QA_TABLES"):
            if not self.isOpt("INSTALL"):
                self.addOpt("INSTALL", "up force")

        if self.isOpt("QA_EXAMPLE"):
            self.dOpt["INSTALL"] =  "up force CORDEX"

        # the location of tables must be known before config files are analysed.
        #self.copyDefaultTables()

        # find valid projects
        '''
        self.definedProjects=[]
        lst=[]
        if not len(self.definedProjects):
            p_prj = os.path.join(self.qa_src, 'tables', 'projects')
            lst = os.listdir(p_prj)

            for itm in lst:
                if os.path.isdir(os.path.join(p_prj, itm)):
                    self.definedProjects.append(itm)

        # if any opt is a SHOW_...
        for key in self.dOpts.keys():
            if key[:4] == 'SHOW':
                self.dOpts['SHOW'] = True
                break
        '''

        return


    def searchQA_CONF_chain(self, fName, lCfgFiles):
        if len(fName) == 0:
            # note that project_as is among the default table names
            if len(self.project_as):
                fName=self.project_as + '_qa.conf'
            else:
                fName=self.project + '_qa.conf'
        elif len(self.project) == 0 and len(self.project_as) == 0:
            p0 = fName.find('_')
            if p0 > -1:
                self.project = fName[:p0]
                self.dOpts['PROJECT'] = self.project

        lCfg=[]
        prfx=os.path.join(self.cfg_opts["QA_TABLES"], 'tables')

        if len(self.project_as):
            prj=self.project_as
            prj_virt=self.project
            vName = fName.replace(prj,prj_virt)
        else:
            prj=self.project

        lCfg.append(fName)
        if len(self.project_as):
            lCfg.append(os.path.join(prfx, prj_virt, vName))
            lCfg.append(os.path.join(prfx, prj_virt, fName))
        lCfg.append(os.path.join(prfx, prj, fName))
        lCfg.append(os.path.join(prfx, 'projects', prj, fName))

        for cfg in lCfg:
            # used before?
            if cfg in lCfgFiles:
                continue

            if os.access(cfg, os.R_OK):
                lCfgFiles.append(cfg)

        return


    def setDefault(self):
        self.ldOpts.append({})
        _ldo = self.ldOpts[len(self.ldOpts)-1]

        _ldo['CFG_FILE']=self.cfg_file
        _ldo['CONDA']=False
        _ldo['HARD_SLEEP_PERIOD']=10
        _ldo['MAIL']='mailx'
        _ldo['NUM_EXEC_THREADS']=1
        #_ldo['PROJECT_DATA']=''
        # _ldo['QA_TABLES']=os.path.join(self.home, 'tables')
        _ldo['QA_HOST']=socket.gethostname()
        _ldo['QA_RESULTS']=os.path.join("/tmp", "QA_Results")
        _ldo['REATTEMPT_LIMIT']=5
        _ldo['SLEEP_PERIOD']=300
        _ldo['QA_EXEC_HOSTS']=_ldo['QA_HOST']

        return


    def setSelLock(self, line):
        # process and return line
        if line[0] == 'S':
            line=line[6:]
        elif line[0] == 'L':
            line=line[4:]
        else:
            return line

        p0=0
        while True: # for '=' or '+=' surrounded by blanks
            if line[p0] == '=':
                line=line[p0:]
                break
            elif line[p0:2] == '+=':
                line=line[p0:]
                break
            elif line[p0] != ' ':
                line = '+=' + line[p0:]
                break

            p0 += 1

        return line


class CfgFile(object):
   '''
   classdocs
   # read,write, modify a configuration file
   '''

   def __init__(self, qaConf):
      '''
      Constructor
      '''

      # traditional config file, usually in the users' home directory
      self.rCP = ConfigParser.RawConfigParser()
      self.rCP.optionxform = str # thus, case-sensitive

      self.qaConf = qaConf
      self.isModified = False
      self.is_read_only=False


   def entry(self, key='', value=''):

      retVal=value

      # inquire/add from/to an existing cfg files
      try:
         # does section exist?
         getVal =self.rCP.get(self.section, key)
      except ConfigParser.NoSectionError:
         if len(value) and not (value == 'd' or value == 'disable'):
               # a new section
               self.rCP.add_section(self.section)
               self.rCP.set(self.section, key, value)
               self.isModified=True
      except ConfigParser.NoOptionError:
         # a new assignment to an existing section
         if len(value) and not (value == 'd' or value == 'disable'):
               self.rCP.set(self.section, key, value)
               self.isModified=True
               self.qaConf.addOpt(key,value)
      else:
         # section and key exist
         if len(value):
            if value == 'd' or value == 'disable':
               # delete a key=value pair
               self.rCP.remove_option(self.section, key)
               retVal=''
               self.isModified=True
            elif getVal != value:
               # replacement
               self.rCP.set(self.section, key, value)
               self.isModified=True
         else:
            retVal=getVal

      return retVal


   def getOpts(self):
      # list of options;
      dOpts={}

      if self.rCP.has_section(self.section):
         lpairs = self.rCP.items(self.section)

         for lp in lpairs:
            dOpts[lp[0]] = lp[1].strip("'")

      return dOpts


   def read_file(self, file, section=''):

      self.section = section

      if len(file):
         self.file = file

      # for .qa.cfg formatted (new)
      isConvert=False
      isOld=False
      isNew=False

      (path, fname)=os.path.split(self.file)

      if fname == '.qa-config.txt':
          old = self.file
          isConvert=True
          isOld=True
          self.is_read_only=True
      elif fname == '.qa.cfg':
          new = self.file
          isNew=True
          self.is_read_only=True
      else:
          self.is_read_only=True

          # as long as installation/update stuff is done by bash scripts,
          # the old config file format has precedence
          old = os.path.join(path, 'config.txt' )
          if os.path.isfile(old):
            isConvert=True
            isOld=True

          new = os.path.join(path, 'qa.cfg' )
          if os.path.isfile(new):
            isNew=True

      if not isNew and not isOld:
          self.rCP.add_section(section)
          return  #from scratch

      if isConvert:
        if not self.is_read_only:
            # at this point old exists
            if isNew:
                old_modTime = qa_util.f_get_mod_time(old)
                new_modTime = qa_util.f_get_mod_time(new)

                if old_modTime > new_modTime:
                    isConvert=True
                else:
                    isConvert=False

        if isConvert:
            # conversion
            with open(old) as r:
                self.isModified=True

                for line in r:
                    line = qa_util.clear_line(line)
                    if len(line):
                        sz1=len(line)-1

                        if line[sz1] == ':' :
                            ny_sect=line[0:sz1] # rm ':'
                            self.rCP.add_section(ny_sect)
                        else:
                            k, v = line.split('=')

                            # some backward compatibility
                            if k == "QA_HOME":
                                k = "QA_TABLES"

                            self.rCP.set(ny_sect, k, repr(v))
        else:
            # read a file created by a self.rCP instance
            self.rCP.read(self.file)
            self.isModified=False

      return


   def write_file(self):
      if self.is_read_only:
          return

      if self.isModified:
         with open(self.file,'wb') as cfg:
            self.rCP.write(cfg)

      return
