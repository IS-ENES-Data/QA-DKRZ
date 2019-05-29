'''
Created on 21.03.2016

@author: hdh
'''

import sys
import os
import glob
import subprocess
import Queue

import qa_util
#from qa_convert_cmor_output import convert_CMOR_output

#from pkg_resources import load_entry_point

class QaExec(object):
    '''
    classdocs
    '''

    def __init__(self, log, qaConf, g_vars):
        # instances for logging results, provided options and a 'python struct'
        self.log = log
        self.qaConf = qaConf
        self.g_vars = g_vars

        self.is_show_call=False
        if qaConf.isOpt('SHOW_CALL'):
            self.is_show_call=True

        self.is_next=False
        if qaConf.isOpt('NEXT'):
            self.is_next=True
            self.next=qaConf.getOpt('NEXT')

        self.capt     = 'CAPT'
        self.capt_len = len(self.capt)
        self.beg      = '-BEG'
        self.end      = '-END'
        self.len_beg  = len(self.beg)
        self.len_end  = len(self.end)

        self.isStatusLine=False
        if self.qaConf.isOpt('STATUS_LINE'):
            self.isStatusLine=True

    def getParamStr(self, t_vars, show=''):
        par =  ' -p ' + t_vars.data_path
        par += show + '-f ' + t_vars.fName
        par += show + '-t ' + self.g_vars.table_path

        #  directives for QA annotations
        par += show + self.get_X_par(t_vars)

        #  directives for CF annotations
        par += show + self.get_X_par(t_vars, inst_ix=1)

        par += show + self.get_CF_par(t_vars)
        par += show + self.get_IN_par(t_vars)
        par += show + self.get_QA_par(t_vars)
        if self.qaConf.isOpt('TIME_LIMIT'):
            par += show + self.get_TC_par(t_vars)
        if self.qaConf.isOpt('FREQ_DIST'):
            par += show + self.get_FD_par(t_vars)

        return par


    def get_CF_par(self, t_vars, inst_ix=0):
        qaConf = self.qaConf

        # combine objects; the space is important
        par = 'CF::0:IN::0:X::1'

        if qaConf.isOpt('CF'):
            par += ':cF=' + qaConf.getOpt('CF')

        if qaConf.isOpt('CF_AREA_TYPES'):
            par += ':cFAT=' + qaConf.getOpt('CF_AREA_TYPES')

        if qaConf.isOpt('CF_STANDARD_NAMES'):
            par += ':cFSN=' + qaConf.getOpt('CF_STANDARD_NAMES')

        if qaConf.isOpt('CF_STD_REGION_NAMES'):
            par += ':cFSRN=' + qaConf.getOpt('CF_STD_REGION_NAMES')

        if qaConf.isOpt('CF_FOLLOW_RECOMMENDATIONS'):
            par += ':fR'

        return par


    def get_IN_par(self, t_vars, inst_ix=0):
        qaConf = self.qaConf

        par = 'IN::0:X::0'

        if qaConf.isOpt('ARITHMETIC_MEAN'):
            par += ':aM'

        if qaConf.isOpt('DISABLE_INF_NAN'):
            par += ':dIN'

        if qaConf.isOpt('EXCLUDE_VARIABLE'):
            par += ':eV=' + qa_util.join(qaConf.getOpt('EXCLUDE_VARIABLE'), sep=',')

        if qaConf.isOpt('UNLIMITED_DIM_NAME'):
            par += ':uDM=' + qa_util.join(qaConf.getOpt('UNLIMITED_DIM_NAME'), sep=',')

        return par


    def get_QA_par(self, t_vars, inst_ix=0):
        qaConf = self.qaConf

        par = 'QA::0:IN::0:CF::0:X::0'
        par += ':f=' + t_vars.qaFN
        par += ':fS=' + t_vars.seq_pos

        if qaConf.isOpt('APPLY_MAXIMUM_DATE_RANGE'):
            par += ':aMDR'
        elif qaConf.isOpt('APPLY_MAXIMUM_DATE_RANGE'):
            par += ':aMDR=' + qaConf.getOpt('APPLY_MAXIMUM_DATE_RANGE')

        if qaConf.isOpt('CHECK_MODE'):
            #val = qaConf.getOpt('CHECK_MODE')
            #val = qa_util.join(val, sep=',')
            #par += ':cM=' + val
            par += ':cM=' + qa_util.join(qaConf.getOpt('CHECK_MODE'), sep=',')

        if qaConf.isOpt('DATA_IN_PRODUCTION'):
            par += ':dIP'

        if qaConf.isOpt('DEFAULT_VALID_MAX'):
            par += ':dVMX'

        if qaConf.isOpt('DEFAULT_VALID_MIN'):
            par += ':dVMN'

        if qaConf.isOpt('DISABLE_CONSISTENCY_CHECK'):
            par += ':dCC'

        if qaConf.isOpt('DISABLE_TIME_BOUNDS_CHECK'):
            par += ':dTB'

        if qaConf.isOpt('EXCLUDE_ATTRIBUTE'):
            par += ':eA=' + qa_util.join(qaConf.getOpt('EXCLUDE_ATTRIBUTE'), sep=',')

        if qaConf.isOpt('FILE_NAME_FREQ_INDEX'):
            par += ':fNFI=' + qaConf.getOpt('FILE_NAME_FREQ_INDEX', bStr=True)

        if qaConf.isOpt('FILE_NAME_VAR_INDEX'):
            par += ':fNVI=' + qaConf.getOpt('FILE_NAME_VAR_INDEX', bStr=True)

        if qaConf.isOpt('FILE_SEQUENCE'):
            par += ':fS=' + qaConf.getOpt('FILE_SEQUENCE')

        if qaConf.isOpt('FREQUENCY'):
            par += ':fq=' + qaConf.getOpt('FREQUENCY')

        if qaConf.isOpt('IGNORE_REF_DATE_ACROSS_EXP'):
            par += ':iRDAE=' + qaConf.getOpt('IGNORE_REF_DATE_ACROSS_EXP')

        if qaConf.isOpt('IGNORE_REFERENCE_DATE'):
            par += ':iRD'

        # these two denote same index in their respective vector
        if qaConf.isOpt('MIP_TABLE_NAME'):
            par += ':mTN=' + qaConf.getOpt('MIP_TABLE_NAME')
        if qaConf.isOpt('MIP_FNAME_TIME_SZ'):
            par += ':mFNTS=' + qaConf.getOpt('MIP_FNAME_TIME_SZ')

        if qaConf.isOpt('NEXT_RECORDS'):
            par += ':nextRecords=' + qaConf.getOpt('NEXT_RECORDS', bStr=True)

        if qaConf.isOpt('NON_REGULAR_TIME_STEP'):
            par += ':nRTS=' + qaConf.getOpt('NON_REGULAR_TIME_STEP')

        if qaConf.isOpt('OUTLIER_TEST'):
            s=','
            par += ':oT=' + qa_util.convert2brace(qaConf.getOpt('OUTLIER_TEST'))

        if qaConf.isOpt('PARENT_EXP_ID'):
            par += ':pEI=' + qaConf.getOpt('PARENT_EXP_ID')

        if qaConf.isOpt('PARENT_EXP_RIP'):
            par += ':pER=' + qaConf.getOpt('PARENT_EXP_RIP')

        if t_vars.post_proc:
            par += ':pP'

        if qaConf.isOpt('PRINT_GREP_MARK'):
            par += ':pGM'

        # PROJECT_TABLE
        par += ':tPr=' + t_vars.pt_name

        if qaConf.isOpt('QA_NCFILE_FLAGS'):
            par += ':qNF=' + qaConf.getOpt('QA_NCFILE_FLAGS')

        if qaConf.isOpt('REPLICATED_RECORD'):
            s=','
            par += ':rR=' + qa_util.convert2brace(qaConf.getOpt('REPLICATED_RECORD'))

        if qaConf.isOpt('TABLE_DRS_CV'):
            par += ':tCV=' + qaConf.getOpt('TABLE_DRS_CV')

        if qaConf.isOpt('TABLE_GCM_NAME'):  # CORDEX
            par += ':tGCM=' + qaConf.getOpt('TABLE_GCM_NAME')

        if qaConf.isOpt('TABLE_RCM_NAME'):  # CORDEX
            par += ':tRCM=' + qaConf.getOpt('TABLE_RCM_NAME')

        if qaConf.isOpt('TABLE_ATT_REQ'):  # CMIP5
            par += ':tAR=' + qaConf.getOpt('TABLE_ATT_REQ')

        if qaConf.isOpt('TABLE_EXPERIMENT'):
            par += ':tX=' + qaConf.getOpt('TABLE_EXPERIMENT')

        if qaConf.isOpt('TABLE_FORCING_DESCRIPT'):
            par += ':tFD=' + qaConf.getOpt('TABLE_FORCING_DESCRIPT')

        if qaConf.isOpt('TABLE_TIME_SCHEDULE'):
            par += ':tTR=' + qaConf.getOpt('TABLE_TIME_SCHEDULE')

        if qaConf.isOpt('TABLE_VAR_REQ'):
            par += ':tVR=' + qaConf.getOpt('TABLE_VAR_REQ')

        if qaConf.isOpt('TABLE_RELAXED_DIM_CONSTRAINT'):
            par += ':tRDC=' + qaConf.getOpt('TABLE_RELAXED_DIM_CONSTRAINT')

        if qaConf.isOpt('TABLE_RELAXED_DIM_CONSTRAINT'):
            par += ':uS'

        return par


    def get_X_par(self, t_vars, inst_ix=0):
        par = 'X::' + str(inst_ix)

        if inst_ix == 0:
            note             = self.qaConf.getOpt('NOTE')
            note_always      = self.qaConf.getOpt('NOTE_ALWAYS')
            note_level_limit = self.qaConf.getOpt('NOTE_LEVEL_LIMIT')
            checklist        = self.qaConf.getOpt('QA_CHECK_LIST')
        else:
            note             = self.qaConf.getOpt('CF_NOTE')
            note_always      = self.qaConf.getOpt('CF_NOTE_ALWAYS')
            note_level_limit = self.qaConf.getOpt('CF_NOTE_LEVEL_LIMIT')
            checklist        = self.qaConf.getOpt('CF_CHECK_LIST')

            # suppress printing of check results
            par += ':nCR'

        if len(note):
            # note is a list
            par += ':note=' + qa_util.convert2brace(note)
        if len(note_always):
            par += ':nA=' + note_always
        if len(note_level_limit):
            par += ':nLL=' + note_level_limit
        if len(checklist):
            par += ':cL=' + checklist

        return par


    def get_TC_par(self, t_vars, inst_ix=0):
        par = 'TC::' + str(inst_ix)
        par += ":e=" + self.qaConf.getOpt('TIME_LIMIT', bStr=True)

        return par


    def get_FD_par(self, t_vars, inst_ix=0):
        qaConf = self.qaConf

        par = 'FD::' + str(inst_ix)
        par += 'useAreaWeight'

        if qaConf.isOpt('FD_PLAIN'):
            par += ':printPlain'

        if qaConf.isOpt('FD_BARS'):
            par += ':printBars'

        #if qaConf.isOpt(''):
        #    par += ':=' + qaConf.getOpt('')

        # A fd-build-file of the same variable at destination has priority.
        # Second choice is a fd-build-file or fd-prop-file in a specified
        # path. Third choice is an explict statement of properties.
        # Default: automatic determination of fd properties.

        # Directive for saving the output to a build-file
        # Note: the effective name will have a time interval appended.
        fName = 'fd_' + t_vars.fBase
        par += ':s=' + fName + '.build'

        if qaConf.isOpt('FD_TIME_PART'):
            par += ':part=' + qaConf.getOpt('FD_TIME_PART')

        # Properties of the freq. dist. Note: current dir is destination.
        files = glob.glob(fName + '*.build')
        if len(files):
            # Rule 1: rebuild, i.e continue a FD
            par += ':r=' + files[0]  # read properties
        elif qaConf.isOpt('FD_PROPERTY_PATH'):
            files = glob.glob(os.path.join( qaConf.getOpt('FD_PROPERTY_PATH'),
                                            fName + '*.build'))

            if len(files):
                # Rule 2a
                par += ':r=' + files[0]
            else:
                files = glob.glob(os.path.join( qaConf.getOpt('FD_PROPERTY_PATH'),
                                                fName + '*.prop'))
                if len(files):
                    # Rule 2b  read properties
                    par += ':rp=' + files[0]
        elif qaConf.isOpt('FD_EXPLICIT_PROPS'):
            props = qaConf.getOpt('FD_EXPLICIT_PROPS').split('/')
            par += ':w=' + props[0] + '@i=' + props[1]

        return par


    def parse_FLAG(self, key, issue, log_entry):
        # replace: " --> '
        while True:
            p0 = issue.find('"')
            if p0 > -1:
                issue = issue[:1] + "'" + issue[2:]
            else:
                break

        add={}
        remainder = self.parse_output(issue, log_entry, dct=add)

        if not 'event' in log_entry:
            # only once
            log_entry['is_event'] = True
            log_entry['event'] = []

        event = {}
        p0 = remainder.find(':')
        event['impact'] = issue[:p0]

        if 'impact' in event:
            p0 = event['impact'].find('-')
            event['tag'] = event['impact'][p0+1:]

            p0 = event['impact'].rfind('-')
            event['impact'] = event['impact'][:p0]
        else:
            event['tag'] = ''
            event['impact'] = ''

        for k in add.keys():
            event[k] = add[k]

        if len(event):
            log_entry['event'].append(event)

        return


    def parse_PERIOD(self, key, issue, log_entry):
        lst = issue.split()
        log_entry[key] = 'period:'
        period=[]

        if len(lst) == 3:
            log_entry[key+self.beg] = 'begin: ' + lst[0]
            log_entry[key+self.end] = 'end: ' + lst[2]
            period.append(lst[0])
            period.append(lst[2])
        elif len(lst) == 2:
            if lst[0] == '-':
                log_entry[key+self.beg] = 'begin: miss'
                log_entry[key+self.end] = 'end: ' + lst[1]
                period.append('')
                period.append(lst[1])
            else:
                log_entry[key+self.beg] = 'begin: ' + lst[1]
                log_entry[key+self.end] = 'end: miss'
                period.append(lst[1])
                period.append('')
        else:
            log_entry[key+self.beg] = 'begin: miss'
            log_entry[key+self.end] = 'end: miss'
            period.append('')
            period.append('')

        log_entry['period'] = period

        return


    def parse_output(self, check_output, log_entry, dct={}):
        # parse the output from  the C++ executable

        # split according to key-words
        keys = [ 'FLAG', 'CAPT', 'TXT', 'CHECK', 'STATUS', 'CREATE', 'TRACK',
                 'PERIOD', 'CPU', 'REAL', 'EMAIL', 'SUBJECT']

        # for multiple, embedded occurrences of keys
        ixs = range(len(keys))

        while True:
            not_found=True

            # find always the key-word that comes first
            p_min=len(check_output)
            for ix in ixs:
                p0 = check_output.find(keys[ix]+self.beg)

                if p0 > -1 and p0 < p_min:
                    p_min = p0
                    p_ix = ix
                    not_found = False
                    if p0 == 0:
						break

            if not_found:
                break

            key = keys[p_ix]

            front='' # frontal residuum
            back=''  # residuum of the back-side

            log_entry['conclusion']=''

            try:
               log_entry['period']
            except:
               log_entry['period']=[]

            #p0 = check_output.find(key+self.beg)
            p0 = p_min
            p1 = check_output.find(key+self.end)

            if p1 > -1:
                front = check_output[0:p0].strip()
                back  = check_output[p1+len(key)+self.len_end:].strip()
                issue = check_output[p0+len(key)+self.len_beg:p1].strip()
                check_output = front + back

                if key == 'CHECK':
                    log_entry['conclusion'] = issue

                elif key == 'PERIOD':
                    self.parse_PERIOD(key, issue, log_entry)

                elif key == 'FLAG':
                    self.parse_FLAG(key, issue, log_entry)

                elif key == 'CAPT':
                    # reached out of parse_FLAG
                    dct['annotation'] = issue.strip()

                elif key == 'TXT':
                    # reached out of parse_FLAG

                    # additional info between TXT, multiple lines
                    # internally separated by ';'
                    issue = issue.strip(' ;')

                    if ';' in issue:
                        dct['info'] = issue.split(';')
                    else:
                        dct['info'] = [ issue ]

                elif key == 'STATUS':
                    log_entry['status'] = issue
                else:
                    log_entry[key] = issue

        return check_output


    def printStatusLine(self, nc=''):
        if self.isStatusLine:
            if len(nc):
                void, f = os.path.split(nc)

                print '\r' + (10+self.qaConf.getOpt("STATUS_LINE_SZ"))*' ',
                print '\rNEXT: ' + f ,

                self.qaConf.addOpt("STATUS_LINE_SZ", len(f) )
            else:
                print '\r' + (10+self.qaConf.getOpt("STATUS_LINE_SZ"))*' ' ,

            sys.stdout.flush()

        return


    def run(self, t_vars):

        os.environ["UDUNITS2_XML_PATH"] = self.g_vars.UDUNITS2_XML_PATH

        qa_lock = os.path.join(t_vars.var_path,
                               'qa_lock_' + t_vars.fBase + '.txt')
        if os.path.isfile(qa_lock):
            return False

        self.nc_file = os.path.join(t_vars.data_path, t_vars.fName)

        self.printStatusLine(nc=self.nc_file)

        if self.qaConf.isOpt('DRY_RUN'):
            print self.nc_file
            return True

        if self.is_show_call:
            param = self.getParamStr(t_vars, '\n')

            fd=open('./param_file.txt', 'w')
            print >> fd, param
            print param
            sys.exit(1)
        else:
            param = self.getParamStr(t_vars, ' ')

        log_entry={}

        qa_util.mkdirP(t_vars.var_path)

        cpp_run = self.g_vars.checkPrg + param
        set_qa_lock = False

        try:
            check_output = subprocess.check_output(cpp_run,
                                                shell=True,
                                                cwd=t_vars.var_path)
        except subprocess.CalledProcessError as e:
            istatus = e.returncode
            set_qa_lock = True

            if istatus == 63:
                return True

            check_output = e.output.strip()

            # atomic variable gets locked
            if istatus > 1:
                set_qa_lock = True

            if istatus == 4:
                pass
                # EMERGENCY STOP
                # isSignalTerm = t
                # issueMail = t

            elif istatus > 4:
                # uncontroled system exception, e.g. segmentation fault
                log_entry['conclusion'] = 'segmentation fault'
                log_entry['is_event'] = True
                log_entry['event'] = []
                event = {}
                event['annotation']   = 'run-time error: segmentation fault'
                event['impact'] = 'L2'
                event['tag']    ='S_1'
                log_entry['event'].append(event)

        else:
            istatus = 0

        if self.qaConf.isOpt('RUN_PREPARE'):
            # run and append output
            check_output += self.run_PrePARE(t_vars)

        entry_id=''

        # prepare the logfile entry
        if istatus < 5:
            self.parse_output(check_output, log_entry)

            entry_id = self.log.append( entry_id,
                                        f           = t_vars.fName,
                                        d_path      = t_vars.data_path,
                                        r_path      = t_vars.var_path,
                                        period      = log_entry['period'],
                                        conclusion  = log_entry['conclusion'],
                                        set_qa_lock = set_qa_lock)

        if 'is_event' in log_entry:
            entry_id = self.log.append(entry_id, is_events=True)

            for eve in log_entry['event']:
                entry_id = self.log.append( entry_id,
                                            annotation  = eve['annotation'],
                                            impact   = eve['impact'],
                                            tag      = eve['tag'])

                if 'info' in eve:
                    entry_id = self.log.append(entry_id, info=eve['info'])

        entry_id = self.log.append(entry_id, status=istatus)

        self.log.write_entry(entry_id,
                       self.g_vars.check_logs_path,
                       t_vars.log_fname)

        if istatus > 1:
            proceed = False
        else:
            proceed = True

        return proceed  # true: proceed with next sub-temporal file


    def run_PrePARE(self, t_vars):
        os.environ["UVCDAT_ANONYMOUS_LOG"] = "no"

        head, file = os.path.split(self.nc_file)
        x_file = file.split('_')

        # get the table_id
        getNC_run = os.path.join( self.g_vars.qa_src, "bin", "getNC_att.x " )
        getNC_run +=  self.nc_file + " --only-value --no-newline table_id"

        try:
            table_id = subprocess.check_output(getNC_run, shell=True)

        except subprocess.CalledProcessError as e:
            # let's try the filename
            if len(x_file) > 1:
                table_id = x_file[1]

        mip_table = os.path.join(self.g_vars.table_path, "cmip6-cmor-tables",
                                 "Tables", "CMIP6_" + table_id + ".json")

        if not os.path.isfile(mip_table):
            print 'Invalid MIP Table for ' + mip_table
            print 'Cancel run of PrePARE'
            return


        # run PrePARE and pipe output to convertPipedCMOR_output
        #pp_run = "/hdh/local/miniconda/envs/cmor/bin/python "

        pp_run = self.qaConf.getOpt("PrePARE")
        pp_run += ' --variable ' + x_file[0]
        pp_run += ' --table-path ' + mip_table + ' ' + self.nc_file
        pp_run += ' 2>&1'

        try:
            ps = subprocess.Popen(pp_run, stdout=subprocess.PIPE, shell=True)
        except subprocess.CalledProcessError as e:
            pass

#        convert = os.path.join(self.g_vars.qa_src, "python",
#                               "qa-dkrz", "convertPipedCMOR_output.py")
        convert="python "
        convert += os.path.join(self.g_vars.qa_src, "python",
                               "qa-dkrz", "qa_convert_cmor_output.py")

        try:
            pp_out = subprocess.check_output(convert, stdin=ps.stdout, shell=True)
        except subprocess.CalledProcessError as e:
            pass

        ps.wait()

        try:
            pp_out
        except:
            pass
        else:
            # replace "..." by <....>
            pp_out2=''
            on=False
            for i in range(len(pp_out)):
                if pp_out[i] == '"':
                    if on:
                        on=False
                        pp_out2 += '>'
                    else:
                        on=True
                        pp_out2 += '<'
                else:
                    pp_out2 += pp_out[i]

        return pp_out2


    def start(self, queue):
        # only called for non-thread operation

        while True:
            try:
                (data_path, fNames, t_vars) = queue.get(False)
            except Queue.Empty:
                break
            else:
                if data_path == '---EOQ---':
                    break

                lenFN_1=len(fNames)-1

                for ix in range(lenFN_1+1):
                    is_post_proc = False

                    if self.is_next:
                        if self.next == 0:
                            break
                        else:
                            self.next -= 1

                    if ix == 0:
                        if ix == lenFN_1:
                            seq_pos = 'x'  # there is only a single file
                            is_post_proc = True
                        else:
                            seq_pos = 'f'
                    elif ix == lenFN_1:
                        seq_pos = 'l'
                        is_post_proc = True
                    else:
                        seq_pos = 's'

                    t_vars.data_path = data_path
                    t_vars.fName = fNames[ix]
                    t_vars.seq_pos = seq_pos
                    t_vars.post_proc = is_post_proc

                    if not self.run(t_vars):
                        break

                queue.task_done()

                # for the previous range == 1
                if self.is_next and self.next == 0:
                    return False

        return True
