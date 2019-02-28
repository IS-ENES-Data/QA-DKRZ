#!/bin/bash -eu
CC="$PREFIX/bin/gcc"
CXX="${PREFIX}/bin/g++"
LIBPATH="${PREFIX}/lib"
LDFLAGS="-Wl,-rpath ${LIBPATH}"

# prepare qa home in opt/qa-dkrz
QA_SRC=${PREFIX}/opt/qa-dkrz
mkdir -vp ${QA_SRC}

echo "CC=${CC}" > install_configure
echo "CXX=${CXX}" >> install_configure
echo "CFLAGS=\"-Wall\"" >> install_configure
echo "CXXFLAGS=\"-g -Wall -std=c++11 -D NC4\"" >> install_configure
echo "LIB=${LIBPATH}" >> install_configure
echo "INCLUDE=\"${PREFIX}/include\"" >> install_configure
cp install_configure ${QA_SRC}

cp -r ./CF-TestSuite ${QA_SRC}
#cp -r ./docs ${QA_SRC}
cp -r ./example ${QA_SRC}
cp -r ./src ${QA_SRC}
cp -r ./include ${QA_SRC}
cp -r ./scripts ${QA_SRC}
cp -r ./python ${QA_SRC}
cp -r ./tables ${QA_SRC}
cp install ${QA_SRC}
cp Makefile ${QA_SRC}
cp README* ${QA_SRC}

touch ${QA_SRC}/.ignore_GitHub # avoids git update!

# run build
#export QA_LIBS="-ludunits2 -lnetcdf -lhdf5_hl -lhdf5 -lz -luuid -lmfhdf -ldf -ljpeg -lssl -lcrypto"
export QA_LIBS="-ludunits2 -lnetcdf -lhdf5_hl -lhdf5 -lz -luuid -lmfhdf"
QA_TABLES=/hdh/hdh/QA_Tables

./install --debug --force --conda-build --net=no --cf --qa-src=$QA_SRC --qa-tables="$QA_TABLES" CF CMIP5 CMIP6 CORDEX # 2> /hdh/hdh/QA-DKRZ/test/asdf

# copy generated files to opt/qa-dkrz
#cp -r ./bin ${QA_SRC}

# write branch,commit_id,tag to install.log
vers="branch=$(git branch | grep '*' | awk '{print $2}')"

declare -a arr
arr=( $(git log --oneline --decorate | grep -m 1 .) )
if [ ${#arr[*]} -gt 0 ] ; then
  vers=${vers}"\nhexa=${arr[0]}"
else
  vers=${vers}"\nhexa=-"
fi

arr=( $( grep -m 1 "version:" conda-recipes/qa-dkrz/meta.yaml ) )
tmp=${arr[$((${#arr[*]}-1))]}
vers="${vers}\ntag=qa-dkrz-${tmp//\'/}"
arr=( $( grep -m 1 "number:" conda-recipes/qa-dkrz/meta.yaml ) )
tmp=${arr[$((${#arr[*]}-1))]}
vers="${vers}-${tmp}"

echo -e ${vers} > ${QA_SRC}/install.log

# install wrapper script in bin/ to call cfchecker and qa-dkrz
cp $RECIPE_DIR/cfchecker-wrapper_env.sh $PREFIX/bin/dkrz-cf-checker.sh
cp $RECIPE_DIR/qa-wrapper_env.sh $PREFIX/bin/qa-dkrz.sh
cp $RECIPE_DIR/qa-py-wrapper_env.sh $PREFIX/bin/qa-dkrz

chmod +x $PREFIX/bin
