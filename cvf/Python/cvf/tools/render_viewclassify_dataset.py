import cvf.bfc as bfc
import cvf.cvrender as cvr
import json
import argparse

def gen_views(nViews, nViewSamples, marginRatio=0):
    assert(nViewSamples>=nViews)
    views=cvr.sampleSphere(nViews)
    samples=cvr.sampleSphere(nViewSamples)

    viewClusters=[]
    for i in range(0,len(views)):
        viewClusters.append([])

    for s in samples:
        mcos=-1
        mi=0
        for i,v in enumerate(views):
            sv_cos=s[0]*v[0]+s[1]*v[1]+s[2]*v[2]
            if sv_cos>mcos:
                mcos=sv_cos
                mi=i
        viewClusters[mi].append((s,mcos))
    #for v in viewClusters:
    for i,v in enumerate(viewClusters):
        v.sort(key=lambda x:x[1],reverse=True)
        viewClusters[i]={'center':views[i],
        'nbrs':v[0:int(len(v)*(1-marginRatio))]
        }

    return viewClusters

import math
import random
import numpy as np
import cv2
import os
import shutil

def render_viewclassify_ds(modelFile, outDir, view_info_path, nViews, nImagesPerView):
    model=cvr.CVRModel(modelFile)
    modelCenter=np.array(model.getCenter())
    sizeBB=model.getSizeBB()
    maxBBSize=max(sizeBB)
    unitScale=2.0/maxBBSize
    eyeDist=4.0/unitScale
    fscale=1.5
    viewSize=[500,500]

    viewClusters=gen_views(nViews,3000)

    json_info = {}
    
    for ci,viewCluster in enumerate(viewClusters):
        print('view {}/{}'.format(ci,len(viewClusters)))
        viewCenter=viewCluster['center']
        viewNbrs=viewCluster['nbrs']
        upDir=[0,0,1] if abs(viewCenter[2])<0.95 else [0,1,0]

        viewDir=outDir+'/%04d/'%ci
        imgDir=viewDir+'/img/'
        maskDir=viewDir+'/mask/'
        os.makedirs(imgDir,exist_ok=True)
        os.makedirs(maskDir,exist_ok=True)

        json_info['%04d'%ci] = viewCenter

        for ii in range(0,nImagesPerView):
            viewDir=viewNbrs[int(random.uniform(0,len(viewNbrs)))][0]
            viewDir=np.array(viewDir)

            eyePos=modelCenter+viewDir*eyeDist

            mats=cvr.CVRMats()
            mats.mModel=cvr.lookat(eyePos[0],eyePos[1],eyePos[2],modelCenter[0],modelCenter[1],modelCenter[2],upDir[0],upDir[1],upDir[2])
            mats.mProjection=cvr.perspectiveF(viewSize[1]*fscale, viewSize, max(1,eyeDist-maxBBSize), eyeDist+maxBBSize)

            angle=2*math.pi*ii/nImagesPerView
            mats.mView = cvr.rotateAngle(angle, [0.0, 0.0, 1.0])

            render=cvr.CVRender(model)
            rr=render.exec(mats,viewSize)
            mask=cvr.getRenderMask(rr.depth)
            #R, T = cvr.decomposeR33T(mats.mModel)

            imname='%04d.png'%ii
            cv2.imwrite(imgDir+imname,rr.img)
            cv2.imwrite(maskDir+imname,mask)

            # cv2.imshow('img',rr.img)
            # cv2.imshow("mask",mask)

            # dimg=cvr.postProRender(rr.img,mask)
            # cv2.imwrite("f:/dimg.png",dimg)

            # cv2.waitKey()
    
    with open(os.path.join(view_info_path, "view_category.json"), "w") as f:
        f.write(json.dumps(json_info, indent=4))
            
def main():
<<<<<<< HEAD
    parser = argparse.ArgumentParser()
    parser.add_argument('--num_classes', type=int, default=240)  # 数据集包含的类别数目
    parser.add_argument('--obj_id', type=int, default=3)  # 训练数据名称
    parser.add_argument('--class_samples', type=int, default=250)  # 每个类别的数据量

    opt = parser.parse_args()

    obj_id = opt.obj_id
    nViews = opt.num_classes
    nImagesPerView = opt.class_samples

    ds_name = 'viewclassify_ycbv_obj{}_margin0_{}_{}'.format(obj_id, nViews, nImagesPerView)
    modelFile='/home/aa/prjs/cp/cosypose/local_data/bop_datasets/ycbv/models/obj_00000{}.ply'.format(obj_id)
    
    outDir='/home/aa/data/3dgen/{}/train'.format(ds_name) 
    view_category_json_path = '/home/aa/data/3dgen/{}'.format(ds_name)

    os.makedirs(outDir, exist_ok=True)
=======
    modelFile='f:/home/aa/data/3dmodels/3ds-model/plane2/plane2.ply'
    
    outDir='f:/home/aa/data/3dgen/viewclassify_01'
>>>>>>> 2260dd086489de480f4d439547c58b5b149f1271
    shutil.rmtree(outDir)

    print('outdir: ' + outDir)
    render_viewclassify_ds(modelFile, outDir, view_category_json_path, nViews,nImagesPerView)


if __name__=='__main__':
    main()
