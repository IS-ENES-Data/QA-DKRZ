
import sys
import qa_util

def convert_CMOR_output(lines):
    blk_marks = [ '!', '=' ]

    blk=[]
    ix=-1
    blkOn=False

    for line in lines:
        if len(line) == 0 or line[0:11] == 'processing:':
            continue

        if line[0:9] == 'Traceback':
            emergency(lines)
            return

        # find the beginning of a block
        count = 0
        for mark in blk_marks:
            c = line.count(mark)
            if c > count:
                count = c

        if count > 5:
            if blkOn:
                blkOn=False
            else:
                blkOn=True
                blk.append([])
                ix += 1

            continue

        if blkOn:
            blk[ix].append(line)

    # remove disturbing sub-strings
    for ix in range(len(blk)):
        for jx in range(len(blk[ix])-1,-1,-1):
            # rm line-feed
            blk[ix][jx] = blk[ix][jx].strip("\t\n !")

            if len(blk[ix][jx]) == 0:
                del  blk[ix][jx]
            else:
                # replace every ';' by '.', because semi-colon serves as line delimiter
                # in qa-exec-check
                if ';' in blk[ix][jx]:
                    blk[ix][jx] = blk[ix][jx].replace(';', '.')

    # re-sort according to annots (terminated py a period)
    annot=[]

    # remove the 'Ceterum censeo Karthaginem esse delendam' phrase
    if len(blk) > 0:
        cicero=blk[len(blk)-1]
        if 'The input file is not CMIP6 compliant' in cicero[0]:
            del blk[len(blk)-1]

    for ix in range(len(blk)):
        annot.append([])
        phrase=''

        for line in blk[ix]:
            last=len(line)-1
            pos0=0

            while True:
                if pos0 == 0 and len(phrase) > 0:
                    phrase += ' '

                pos = line.find('.', pos0)
                if pos == -1:
                    phrase += line[pos0:]
                    break
                else:
                    # there is at least one period somewhere in the line
                    if last == pos:
                        # current line ends with a period
                        phrase=phrase + line[pos0:pos+1]
                        # annot[ix].append(phrase + line[pos0:pos+1])
                        phrase=''
                        break
                    elif line[pos+1] == ' ':
                        # current line contains a period terminating the current annot
                        phrase=phrase + line[pos0:pos+1]
                        #annot[ix].append(phrase + line[pos0:pos+1])
                        phrase=''
                        pos += 1
                    else:
                        phrase += line[pos0:pos+1]

                    pos0 = pos+1


        if len(phrase):
            # harmonise: 'The entry "varname" could not be found in CMOR table'
            if phrase.find('could not be found in CMOR table') > -1:
               words=phrase.split()
               if words[0] == 'The':
                  if words[1] == 'entry':
                     phrase=phrase.replace(words[2], '<*>')

            annot[ix].append(phrase)

    for ix in range(len(annot)):
        # the first annot of each annotation group acts a caption
        # the tag equals the length of the caption

        s = qa_util.replace_clasped(annot[ix][0], '<>', '*')

        md5 = qa_util.get_md5sum(s[0:50])

        flag = ' FLAG-BEGL1-' + md5[0:4] + ':CAPT-BEG'
        flag += 'CMOR ' + annot[ix][0] + 'CAPT-END'

        annot[ix][0] = flag
        sz = len(annot[ix])

        if sz > 1:
            annot[ix][0] += 'TXT-BEG'
            for jx in range(1,sz):
                if jx > 1:
                    annot[ix][0] += ';'

                annot[ix][0] += annot[ix][jx]

            annot[ix][0] += 'TXT-END'

            del annot[ix][1:]

        annot[ix][0] += 'FLAG-END'
        annot[ix][0] = annot[ix][0].replace('"','\"')


    for ix in range(len(annot)):
        print annot[ix][0],

    return

def emergency(lines):
    flag = ' FLAG-BEG:CAPT-BEG'
    flag += 'CMOR ' + 'Traceback issued' + 'CAPT-END'

    annot = flag
    sz = len(lines)

    annot += 'TXT-BEG'
    for jx in range(1,sz):
        if jx > 0:
            annot += ';'

        annot += lines[jx]

    annot += 'TXT-END'

    annot += 'FLAG-END'
    annot = annot.replace('"','\"')

    print annot

    return

# -------- main --------

if __name__ == '__main__':
    lines=[]
    if len(sys.argv) == 1:
        for line in sys.stdin:
            lines.append(line)
    elif len(sys.argv) == 2:
        with open( sys.argv[1]) as fd:
            for line in fd:
                line = line.rstrip(' \n')
                if len(line):
                    lines.append(line)

    convert_CMOR_output(lines)
