if [ -e $1 ]; then
echo HELP:
echo ./add-c-file.sh \[filename\]
echo filename: file name without extension \(e.g. \"hello\" rather than \"hello.c\"\)
else
echo \$\(ARCHDIR\)/$1.o \\ >> ./kernel/arch/i386/make.config
fi
