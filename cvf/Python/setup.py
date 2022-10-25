import os
from distutils.sysconfig import get_python_lib

libDir=get_python_lib()
curDir=os.path.dirname(__file__)
curDir=os.path.abspath(curDir)

print('libDir='+libDir)
print('curDir='+curDir)

with open(libDir+'/'+'cvf.pth','w') as f:
        f.write(curDir+'\n')

'''
libs=['bfc']

for lib in libs:
    with open(libDir+'/'+lib+'.pth','w') as f:
        f.write(curDir+'\n')
'''
