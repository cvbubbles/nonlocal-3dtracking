import cvf.bfc as bfc
import cvf.cvrender as cvr
import json
import argparse
import numpy as np
import math
import random
import numpy as np
import cv2
import os
import shutil
import csv


"""
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



def render_viewclassify_ds(modelFile, outDir, view_info_path, nViews, nImagesPerView):
    model=cvr.CVRModel(modelFile)
    modelCenter=np.array(model.getCenter())
    sizeBB=model.getSizeBB()
    maxBBSize=max(sizeBB)
    unitScale=2.0/maxBBSize
    eyeDist=4.0/unitScale
    fscale=1.5
    viewSize=[300,300]

    viewClusters=gen_views(nViews,3000)
    
    obj_name = modelFile.split('/')[-1].replace('.ply', '')

    json_info = {}
    
    for ci,viewCluster in enumerate(viewClusters):
        print('view {}/{}'.format(ci,len(viewClusters)))
        viewCenter=viewCluster['center']
        viewNbrs=viewCluster['nbrs']
        upDir=[0,0,1] if abs(viewCenter[2])<0.95 else [0,1,0]

        imgDir=outDir+'/img/'
        maskDir=outDir+'/mask/'
        os.makedirs(imgDir,exist_ok=True)
        os.makedirs(maskDir,exist_ok=True)

        for ii in range(0, nImagesPerView**0.5):
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
            R, T = cvr.decomposeR33T(mats.mModel * mats.mView)
            
            imname = obj_name + '_' + str(ci) + '_%04d.png'%ii
            cv2.imwrite(imgDir+imname,rr.img)
            cv2.imwrite(maskDir+imname,mask)
            
            
            
            
            #json_info[imname.replace('.png', '')] = {'R': R.tolist(), 'T': T}

            # cv2.imshow('img',rr.img)
            # cv2.imshow("mask",mask)

            # dimg=cvr.postProRender(rr.img,mask)
            # cv2.imwrite("f:/dimg.png",dimg)

            # cv2.waitKey()
    
    with open(os.path.join(view_info_path, "view_category.json"), "w") as f:
        f.write(json.dumps(json_info, indent=4))
            
def main():
    
    nViews = 15
    nImagesPerView = 225
    
    ds_name = 'idea_ycbv_roate'
    outDir = '/home/aa/data/3dgen/{}/train'.format(ds_name) 
    view_category_json_path = '/home/aa/data/3dgen/{}'.format(ds_name)

    os.makedirs(outDir, exist_ok=True)
    shutil.rmtree(outDir)
    
    for i in range(21):
        obj_id = i+1
        modelFile = '/home/aa/prjs/cp/cosypose/local_data/bop_datasets/ycbv/models/obj_%06d.ply'%obj_id
        
        print('outdir: ' + outDir)
        render_viewclassify_ds(modelFile, outDir, view_category_json_path, nViews, nImagesPerView)


if __name__=='__main__':
    main()
"""

def render_angel_ds_singleV4(modelFile, outDir, nImagesPerView, csv_writer, category_info):
    model_name = modelFile.split('/')[-1].replace('.ply', '')
    model = cvr.CVRModel(modelFile)  # 加载模型
    modelCenter = np.array(model.getCenter())  # 获取模型中心
    sizeBB = model.getSizeBB()
    maxBBSize = max(sizeBB)
    unitScale = 2.0 / maxBBSize
    eyeDist = 4.0 / unitScale
    fscale = 1.5
    viewSize = [300, 300]

    categories = []
    views = []
    for category, vector in category_info.items():
        categories.append(category)
        views.append(vector)
        
    samples = cvr.sampleSphere(3000)  # 采样3000个视角
    
    marginRatio = 0;
    
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
    for i,v in enumerate(viewClusters):
        v.sort(key=lambda x:x[1],reverse=True)
        viewClusters[i]={'center':views[i],
        'nbrs':v[0:int(len(v)*(1-marginRatio))]
        }
    views_info = [viewClusters[i+7] for i in range(0,len(viewClusters),12)]
    for ci, viewCluster in enumerate(views_info):
        viewCenter = viewCluster['center']
        viewNbrs = viewCluster['nbrs']
        upDir = [0,0,1] if abs(viewCenter[2]) < 0.95 else [0,1,0]
        
        for ii in range(0, nImagesPerView):
           
            viewDir = viewNbrs[int(random.uniform(0,len(viewNbrs)))][0]
            viewDir = np.array(viewDir)

            eyePos = modelCenter+viewDir*eyeDist
    
            # 旋转图1
            mats = cvr.CVRMats()
            mats.mModel = cvr.lookat(eyePos[0],eyePos[1],eyePos[2],modelCenter[0],modelCenter[1],modelCenter[2],upDir[0],upDir[1],upDir[2])
            mats.mProjection = cvr.perspectiveF(viewSize[1]*fscale, viewSize, max(1,eyeDist-maxBBSize), eyeDist+maxBBSize)
            render_angle = 2*math.pi*ii/nImagesPerView
            #render_angle = 2*math.pi*current/(samples-1)
            #render_angle -= math.pi
            mats.mView = cvr.rotateAngle(render_angle, [0.0, 0.0, 1.0])
            # 渲染图片
            render = cvr.CVRender(model)
            rr = render.exec(mats, viewSize)
            mask = cvr.getRenderMask(rr.depth)

            render_image = '{}_{}_{}_{}.png'.format(int(model_name.replace('obj_', '')), ci, ii, 'origin_' + str(round(render_angle, 4)))
            cv2.imwrite(os.path.join(outDir, 'img', render_image), rr.img)
            cv2.imwrite(os.path.join(outDir, 'mask', render_image), mask)
            print("Current: {} {} {} angle: {}".format(model_name,  ci, ii, np.rad2deg(render_angle)))
            
            
def load_json(json_path: str):
    with open(json_path,'r') as json_file:
        json_dict = json.load(json_file)

    return json_dict

def sample_all(args):
    
    num_objs = args.num_objs
    #class_samples = args.class_samples
    nImagesPerView = 60  # 每个视角采取的样本数量

    ds_name = args.ds_name

    # 生成图片的保存路径
    outDir = '/home/aa/data/3dgen/{}/{}'.format(ds_name, opt.sub_ds) 
    csv_save_path = '/home/aa/data/3dgen/{}/{}/{}'.format(ds_name, opt.sub_ds, 'label.csv') 
    os.makedirs(outDir, exist_ok=True)
    shutil.rmtree(outDir)
    #os.makedirs(outDir, exist_ok=True)

    # 创建相应的文件夹
    imgDir = outDir + '/img/'
    maskDir = outDir + '/mask/'
    os.makedirs(imgDir, exist_ok=True)
    os.makedirs(maskDir, exist_ok=True)
    
    # 创建文件对象
    csv_file = open(csv_save_path, 'w', encoding='utf-8', newline="")

    # 基于文件对象构建 csv写入对象
    csv_writer = csv.writer(csv_file)

    csv_writer.writerow(["image1", "image2", "theta_diff"])

    # 遍历所有物体
    for obj_id in range(num_objs):
        obj_id += 1
        # 模型路径
        modelFile = '/home/aa/prjs/cp/cosypose/local_data/bop_datasets/ycbv/models/obj_' + '%06d'%obj_id + '.ply'
        category_path = '/home/aa/data/3dgen/viewclassify_ycbv_margin0_240_250/train/{}/view_category.json'.format(str(obj_id))
        assert os.path.exists(category_path), 'file: {} error'.format(category_path)

        category_dict = load_json(category_path)
        #render_angel_ds_singleV3(modelFile, outDir, nImagesPerView, csv_writer, category_info=category_dict)
        render_angel_ds_singleV4(modelFile, outDir, nImagesPerView, csv_writer, category_info=category_dict)
        
    

if __name__=='__main__':
    # 1 5 6
    parser = argparse.ArgumentParser()
    parser.add_argument('--ds_name', type=str, default='idea_ycbv_roate')  # 数据集包含的类别数目
    parser.add_argument('--num_objs', type=int, default=21)  # 数据集包含的类别数目
    parser.add_argument('--class_samples', type=int, default=1500)  # 每个类别的数据量
    parser.add_argument('--sub_ds', type=str, default='test')  # 

    opt = parser.parse_args()
    sample_all(opt)
