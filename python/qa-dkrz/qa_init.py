'''
Created on 21.03.2016


@author: hdh
'''

import sys
import os
import glob
import shutil
import subprocess
import re

import qa_util

def cpTables(key, fTable, tTable, tTable_path, qaConf, prj_from, prj_to, pDir):

    qaTables = qaConf.getOpt('QA_TABLES')

    if len(pDir) == 0:
        if prj_from == prj_to:
            # regular precedence of paths, i.e. highest first
            if os.path.isfile(fTable):
                # absolute path
                pDir.append(fTable)
            else:
                # local file
                pwd = os.getcwd()
                if os.path.isfile( os.path.join(pwd, fTable) ):
                    pDir.append(os.path.join(pwd, fTable))

            if qaConf.isOpt('USE_STRICT') and prj_from == prj_to:
                # USE_STRICT: only the project's default directory
                pDir.append(os.path.join('tables', 'projects', prj_from) )
            else:
                pDir.append('tables')
                pDir.append(os.path.join('tables', prj_from) )
        else:
            # the only possible place
            pDir.append(os.path.join('tables', prj_from) )

    if prj_from == prj_to:
        if prj_from != 'CF':
            if fTable[0:3] == 'CF_' or fTable[0:3] == 'cf-':
                pDir.append( os.path.join('tables', 'projects', 'CF') )

    if 'CHECK_LIST' in key:
        # concatenate existing files
        # is any of the files newer than the destination?
        dest = os.path.join(tTable_path, tTable)
        dest_modTime = qa_util.f_get_mod_time(dest)

        for pD in pDir:
            src = os.path.join(qaTables, pD, fTable)
            if os.path.isfile(src):
                if prj_from == prj_to:
                    dest= os.path.join(tTable_path, tTable)
                else:
                    dest= os.path.join(tTable_path, fTable)
                    tmp=os.path.join(tTable_path, tTable)

                    try:
                        os.rename(tmp, dest)
                    except:
                        print 'qa_init.cpTables(): could not rename(tmp, dest)'
                    else:
                        # the file of PROJECT_VIRT first
                        d = [src, dest]
                        qa_util.cat(d, dest)

                    # exchange properties of a corresponding project file
                    qaConf.addOpt(key, fTable)

                break

    else:
        # just copy the file with highest precedence; there should only
        # be a single one for each kind of table
        for pD in pDir:
            src = os.path.join(qaTables, pD, fTable)

            if os.path.isfile(src):
                if prj_from == prj_to:
                    dest= os.path.join(tTable_path, tTable)
                else:
                    dest= os.path.join(tTable_path, fTable)

                    # exchange properties of a corresponding project file
                    qaConf.addOpt(key, fTable)

                if qa_util.f_get_mod_time(src) > qa_util.f_get_mod_time(dest):
                    shutil.copyfile(src,dest)

                break

    return

def ext_tables_dialog(QA_SRC, prj=''):

    text = '\nExternal tables are missing or not up to date.'
    text += '\nPlease, run: '
    text += os.path.join(QA_SRC, 'install')
    text += ' up '
    if len(prj):
        text +=  prj
    else:
        text += ' your-project'

    print text
    return

def init_session(g_vars, qaConf):
    g_vars.curr_date = qa_util.date()
    g_vars.session   = g_vars.curr_date
    g_vars.session_logdir = os.path.join(g_vars.res_dir_path,
                                'session_logs', g_vars.curr_date)

    qaConf.addOpt('SESSION', g_vars.session)
    qaConf.addOpt('SESSION_LOGDIR', g_vars.session_logdir)

    if qaConf.isOpt('SHOW'):
        return

    qa_util.mkdirP(g_vars.session_logdir) # error --> exit

    with open(os.path.join(g_vars.session_logdir,
                            'pid.' + g_vars.pid), 'w') as fd:
        fd.write( os.getcwd() + '\n')
        for a in sys.argv:
            fd.write(' ' + a)
        fd.write(' --fpid ' + str(g_vars.pid) + '\n')

    return


def init_tables(g_vars, qaConf):
    TP='TABLE_PATH'

    g_vars.table_path = qaConf.getOpt(TP)
    tp_sz = len(g_vars.table_path)

    if tp_sz:
        # user-defined
        if g_vars.table_path[tp_sz-1] == '/':
            g_vars.table_path = g_vars.table_path[:tp_sz-1]
    else:
        g_vars.table_path = os.path.join(g_vars.res_dir_path, 'tables')

    qa_util.mkdirP(g_vars.table_path)
    qaConf.addOpt(TP, g_vars.table_path)

    # Precedence of path search for tables:
    #
    #   tables/${PROJECT_VIRT}
    #   tables/${PROJECT}
    #   tables/projects/${PROJECT}

    # 1) default tables are provided in QA_SRC/tables/projects/PROJECT
    # 2) do not edit default tables; they are overwritten by updates
    # 3) Option TABLE_AUTO_UPDATE would search for updates for projects/PROJECT
    # 4) option USE_STRICT discards non-default tables.

    # rsync of default tables
    rsync_default_tables(g_vars, qaConf)

    # collect all table names in a list
    tables={}

    for key in qaConf.dOpts.keys():
        tName = ''

        # project tables
        if 'TABLE' in key:
            tName = qaConf.getOpt(key)
        elif 'CHECK_LIST' in key:
            tName = qaConf.getOpt(key)
        elif 'CF_' in key and key[3] != 'F':
            tName = qaConf.getOpt(key)

        if len(tName):
            regExp = re.match(r'^[a-zA-Z0-9\._-]*$', tName)
            if regExp:
                tables[key] = tName

    qaConf.addOpt('TABLES', tables)

    prj_to=qaConf.getOpt('PROJECT')

    pDir=[]
    for key in tables.keys():
        # for genuine projects
        cpTables( key, tables[key], tables[key], g_vars.table_path, qaConf,
                  prj_to, prj_to, pDir)


    if qaConf.isOpt('PROJECT_VIRT'):
        prj_from = qaConf.getOpt('PROJECT_VIRT')

        # find corresponding tables in the virtual project
        vTables={}
        pHT=os.path.join(qaConf.getOpt('QA_TABLES'), 'tables', prj_from)
        for key in tables.keys():
            name = tables[key]
            if prj_to in name:
                name = name.replace(prj_to, prj_from)

                if os.path.isfile( os.path.join(pHT, name) ):
                    vTables[key] = name

        pDir=[pHT]
        for key in vTables.keys():
            cpTables( key, vTables[key], tables[key], g_vars.table_path, qaConf,
                      prj_from, prj_to, pDir )

    return


def rsync_default_tables(g_vars, qaConf):
   # copy the default tables to the current session location

   if not os.path.isdir(g_vars.table_path):
      qa_util.mkdirP(g_vars.table_path)

   src_0=os.path.join(qaConf.getOpt('QA_TABLES'), 'tables')
   if not src_0:
      src_0=os.path.join(qaConf.getOpt('QA_SRC'), 'tables')

   prj=qaConf.getOpt('PROJECT')  # '' wqhen not defined
   #elif qaConf.isOpt('DEFAULT_PROJECT'):
   #   prj=qaConf.getOpt('DEFAULT_PROJECT')
   #else:

   src=''  # prevent a fatal state below
   if prj:
      # with trailing '/'
      src=os.path.join(src_0, 'projects', prj, '')

   if not ( prj or os.path.isdir(src) ):
      print 'no project specified'
      sys.exit(1)

   dest=g_vars.table_path

   rsync_cmd_0='rsync' + ' -lrtuz ' + ' --copy-links' \
               + " --exclude='*~'" + " --exclude='.*'" + " --exclude='*_qa.conf'" \
               + " --exclude='IS-ENES-Data.github.io'"

   rsync_cmd = rsync_cmd_0 + ' ' + src + ' ' + dest

   try:
      subprocess.call(rsync_cmd, shell=True)
   except:
      print 'could not rsync ' + src + ' --> ' + dest
      sys.exit(1)

   # special: CF tables
   if prj != 'CF':
      src=os.path.join(src_0, 'projects', 'CF', '')

   rsync_cmd = rsync_cmd_0 + ' ' + src + ' ' + dest

   try:
      subprocess.call(rsync_cmd, shell=True)
   except:
      print 'could not rsync ' + src + ' --> ' + dest
      sys.exit(1)

   return


def run(log, g_vars, qaConf):
    #g_vars.TTY = os.ttyname(0)

    # update external tables and in case of running qa_dkrz.py from
    # sources update C++ executables
    # run_install(qaConf)

    if qaConf.isOpt('NUM_EXEC_THREADS'):
        g_vars.thread_num = \
            sum( qa_util.mk_list(qaConf.getOpt('NUM_EXEC_THREADS')) )
    else:
        g_vars.thread_num = 1

    g_vars.res_dir_path = qaConf.getOpt('QA_RESULTS')
    g_vars.project_data_path = qaConf.getOpt('PROJECT_DATA')
    g_vars.prj_dp_len = len(g_vars.project_data_path)

    init_session(g_vars, qaConf)

    g_vars.check_logs_path = os.path.join(g_vars.res_dir_path, 'check_logs')

    g_vars.cs_enable = False
    if qaConf.isOpt('CHECKSUM'):
        g_vars.cs_enable = True
        if qaConf.isOpt('CHECKSUM'):
            g_vars.cs_type = 'md5'
        else:
            g_vars.cs_type = qaConf.getOpt('CHECKSUM')

        cs_dir = qaConf.getOpt('CS_DIR')
        if len(cs_dir) == 0:
            cs_dir='cs_table'
        g_vars.cs_dir = os.path.join(g_vars.res_dir_path, cs_dir)

    qaConf.addOpt('LOG_FNAME_DIR', g_vars.check_logs_path)
    qa_util.mkdirP(g_vars.check_logs_path) # error --> exit

    qa_util.mkdirP(os.path.join(g_vars.res_dir_path, 'data')) # error --> exit

    # some more settings
    if not qaConf.isOpt('ZOMBIE_LIMIT'):
        qaConf.addOpt('ZOMBIE_LIMIT', 3600)

    if not qaConf.isOpt('CHECKSUM'):
        if qaConf.isOpt('CS_STAND_ALONE') or qaConf.isOpt('CS_DIR'):
            qaConf.addOpt('CHECKSUM', True)

    # save current version id to the cfg-file
    '''
    if qaConf.isOpt('QA_VERSION'):
      qv=qaConf.getOpt('QA_VERSION')
    else:
      qv = qa_util.get_curr_revision(g_vars.qa_src, g_vars.isConda)
      qaConf.cfg.entry(key='QA_VERSION', value=qv)
    g_vars.qa_revision = qv
    '''

    # abort criteria
    isAbort=False
    abortStr=''
    if len( qaConf.getOpt('PROJECT') ) == 0:
        isAbort=True
        abortStr='PROJECT'
    elif not qaConf.isOpt('PROJECT_DATA'):
        if not qaConf.isOpt('SELECT_PATH_LIST'):
            isAbort=True
            abortStr='PROJECT_DATA or explicit filename'

    if isAbort:
        print abortStr + ' was not defined.'
        print ' Did you miss to provide any of the sufficient options,'
        print ' e.g. -f task-file, QA_CONF, -P PROJECT + filename?'
        sys.exit(1)

    # table path and copy of tables for operational runs
    init_tables(g_vars, qaConf)

    # unique exp_name and table_names are defined by indices of path components
    g_vars.drs_path_base = qaConf.getOpt('DRS_PATH_BASE')

    qa_util.get_experiment_name(g_vars, qaConf, isInit=True)
    qa_util.get_project_table_name(g_vars, qaConf, isInit=True)

    # enable clearance of logfile entries by the CLEAR option
    if qaConf.isOpt('CLEAR_LOGFILE'):
        g_vars.clear_logfile = True
    else:
        g_vars.clear_logfile = False

    g_vars.ignore_temp_files = qaConf.isOpt('IGNORE_TEMP_FILES')
    g_vars.syncFilePrg = os.path.join(g_vars.qa_src, 'bin', 'syncFiles.x')
    g_vars.validNcPrg = os.path.join(g_vars.qa_src, 'bin', 'testValidNC.x')
    g_vars.checkPrg = os.path.join(g_vars.qa_src, 'bin',
                                     'qA-' + qaConf.getOpt('PROJECT') + '.x')

    if not os.access(g_vars.syncFilePrg, os.X_OK):
        print g_vars.syncFilePrg + ' is not executable'
        sys.exit(1)
    if not os.access(g_vars.validNcPrg, os.X_OK):
        print g_vars.validNcPrg + ' is not executable'
        sys.exit(1)
    if not os.access(g_vars.checkPrg, os.X_OK):
        print g_vars.checkPrg + ' is not executable'
        sys.exit(1)

    g_vars.anyProgress = False

    return


def run_install(qa_src):

    if not ( sys.argv[1] == 'install' or sys.argv[1] == '--install'):
        return

    p = os.path.join(qa_src, 'install --no-progress-wheel')

    for pa in sys.argv[2:]:
        if len(pa):
            p += ' ' + pa

    try:
        subprocess.check_call(p, shell=True)
    except subprocess.CalledProcessError as e:
        if e.returncode == 41:
            sys.exit(41)

        print '\ncould not run install.'

    sys.exit(0)

    return
