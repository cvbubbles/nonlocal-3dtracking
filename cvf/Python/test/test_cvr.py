
import os
import sys
import numpy as np
import cv2
import time

#envs=os.environ.get("PATH")
#os.environ['PATH']=envs+';F:/dev/cvfx/assim410/bin-v140/x64/release/;F:/dev/cvfx/opencv3413/bin-v140/x64/Release/;F:/dev/cvfx/bin/x64/;D:/setup/Anaconda3/;'

import cvf.cvrender as cvr

#cvr.init("")
#dr=cvr.DatasetRender()
#img=np.zeros([320,240],np.uint8)
#rr=dr.render(img,1,0)

a=cvr.I()

viewSize=[640,480]
K=cvr.defaultK(viewSize,1.5)
P=cvr.fromK(K,viewSize,1,100)
print(K,P)

#mats=cvr.CVRMats([640,480])

#print(mats.mProjection)

model=cvr.CVRModel('/home/aa/data/3dmodels/test/cat.obj')
render=cvr.CVRender(model)

mats=cvr.CVRMats(model,viewSize)
rr=render.exec(mats, viewSize)

#cvr.mdshow("model",model)
#cvr.waitKey()

start=time.perf_counter()
rr=render.exec(mats,viewSize,cvr.CVRM_IMAGE|cvr.CVRM_DEPTH)
print('time={}ms'.format((time.perf_counter()-start)*1000))

cv2.imwrite('./out.jpg',rr.img)

#cv2.imshow("img",rr.img)
#cv2.waitKey()



