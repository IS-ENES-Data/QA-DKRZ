#!/bin/bash

get_QA_path()
{
   local name p p0 pth

   p0=$0
   pth=$PWD

   while : ; do
      name=${p0##*/}
      p0=${p0%/*}/

      if [ ${p0} = '/' ] ; then
         echo "there is something wrong with ${p0}"
         exit 1
      elif [ ${p0:0:1} = '/' ] ; then
         pth=${p0%/}
      else
         # please, mind the trailing '/'
         if [ ${p0:0:2} = '..' ] ; then
            p0=${pth%/*}/${p0#*/}
         elif [ ${p0:0:1} = '.' ] ; then
            p0=${pth}/${p0#*/}
         else
            # relative path, whatever
            p0=${pth}/$p0
         fi

         p0=${p0}${name}
      fi

      if [ -h $p0 ] ; then
        # resolve symbolic links
        p0=$( ls -l $p0 2> /dev/null | awk '{print $11}')

        if [ ${p0:0:1} = '/'} ] ; then
           pth=${p0%/*}
           continue
        fi
      else
        break
      fi
   done

   # only the directory
   p0=${p0%/*}

   test ${p0##*/} = 'bin' && p0=${p0%/bin}/opt/qa-dkrz/

   QA_PATH=${p0%/*}

   return
}

args=( $* )
if [ "${args[0]%%=*}" = 'install' -o "${args[0]%%=*}" = '--install' ] ; then
  if [ "${args[0]#*=}" != "${args[0]}" ] ; then
     args[0]="${args[0]#*=}"
  else
     args[0]=
  fi
  isInstall=t
fi

get_QA_path

unset LD_LIBRARY_PATH
#export LD_LIBRARY_PATH=${QA_PATH}/lib

if [ ${isInstall:-f} = t ] ; then
  exec ${QA_PATH}/opt/qa-dkrz/install ${args[*]}
else
  exec ${QA_PATH}/opt/qa-dkrz/scripts/qa-dkrz $*
fi

