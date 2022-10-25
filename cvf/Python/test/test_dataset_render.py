
import os
import sys

envs=os.environ.get("PATH")
os.environ['PATH']=envs+';F:/dev/cvfx/assim410/bin-v140/x64/release/;F:/dev/cvfx/opencv3413/bin-v140/x64/Release/;F:/dev/cvfx/bin/x64/;D:/setup/Anaconda3/;'

import cv2
from cv2 import data
from skimage import measurecd

import random
import numpy as np

import pycocotools as coco
import pycocotools.mask

def categories(label, label_id):
    category = {}
    category['supercategory'] = 'component'
    category['id'] = label_id
    category['name'] = label
    return category

def get_category_list(label_list):
    category_list=[]
    for i,label in enumerate(label_list):
        category_list.append(categories(label,i+1))
    return category_list


def get_image_info(img, id, fileName):
    image = {}
    image['height'] = img.shape[0]
    image['width'] = img.shape[1]
    image['id'] = id
    image['file_name'] = fileName
    return image
    

def annotations_from_rect(bbox, category_id, image_id, object_id):
    annotation = {}
    x0,x1,y0,y1=bbox[0],bbox[0]+bbox[2],bbox[1],bbox[1]+bbox[3]
    annotation['segmentation'] = [x0,y0,x0,y1,x1,y1,x1,y0]
    annotation['iscrowd'] = 0
    annotation['image_id'] = image_id
    annotation['bbox'] = bbox
    annotation['area'] = bbox[2]*bbox[3]
    annotation['category_id'] = category_id
    annotation['id'] = object_id
    return annotation



def annotations_from_mask(obj_mask, category_id, image_id, object_id):
    encoded_mask = coco.mask.encode(np.asfortranarray(obj_mask))
    contours = measure.find_contours(obj_mask, 0.5)

    segs=[]
    for contour in contours:
        contour=np.flip(contour,axis=1)
        seg=contour.ravel().tolist()
        segs.append(seg)

    annotation = {}
    annotation['segmentation'] = segs
    annotation['iscrowd'] = 0
    annotation['image_id'] = image_id
    annotation['bbox'] = coco.mask.toBbox(encoded_mask)
    annotation['area'] = coco.mask.area(encoded_mask)
    annotation['category_id'] = category_id
    annotation['id'] = object_id
    return annotation

def rand_box(imsize, minSize, maxSize):
    x=0
    y=0
    while True:
        #x=random.randint(0,imsize[0])
        #y=random.randint(0,imsize[1])
        x=int(random.uniform(0,imsize[0]))
        y=int(random.uniform(0,imsize[1]))
        if min(x,y)>minSize:
            break
    size=random.randint(minSize,min(x,y,maxSize))
    return [x-size, y-size, size]


import cvf.cvrender as cvr
import json

class GenDet2dDataset:
    def __init__(self, imageFiles, modelFiles, labelList) -> None:
        self.dr=cvr.DatasetRender()
        self.dr.loadModels(modelFiles)

        self.imageFiles=imageFiles
        self.modelFiles=modelFiles
        self.category_list=get_category_list(labelList)
        self.labelList=labelList

    def gen(self, outDir, setName, nImages, maxModelsPerImage):
        imageDir=outDir+setName+'/'
        annDir=outDir+'annotations/'
        os.makedirs(imageDir,exist_ok=True)
        os.makedirs(annDir,exist_ok=True)

        category_list=self.category_list
        images_list=[]
        annotations_list=[]

        #idx_of_all_models=list(range(len(label_list)))
        n_all_models=len(self.labelList)
        max_models_perim=min(n_all_models,maxModelsPerImage)
        nobjs=0
        nimgs=0
        idList=[i for i in range(0,n_all_models)]
        for n in range(0,nImages):
            img=cv2.imread(self.imageFiles[random.randint(0,len(self.imageFiles)-1)])
            if img is None:
                continue
            #cv2.imshow("img",img)
            dsize=800.0/max(img.shape)*np.asarray(img.shape)
            dsize=dsize.astype(np.int32)
            dsize=(dsize[1],dsize[0])
            img=cv2.resize(img,dsize)

            obj_list=[]
            size_list=[]
            center_list=[]
            nobjs_cur=random.randint(1,max_models_perim)
            random.shuffle(idList)
            #print(nobjs_cur)
            for i in range(0,nobjs_cur):
                #obj_list.append(random.randint(0,n_all_models-1))
                obj_list.append(idList[i])
                #size_list.append(random.randint(100,min(dsize)))
                #center_list.append([random.randint(0,dsize[0]),random.randint(0,dsize[1])])
                imaxSize=min(int(min(dsize)*4/5),400)
                size=int(random.uniform(150,imaxSize))
                rbb=rand_box(dsize,size,size+1)
                size_list.append(rbb[2])
                center_list.append([int(rbb[0]+rbb[2]/2),int(rbb[1]+rbb[2]/2)])
        
            rr=self.dr.renderToImage(img,obj_list,sizes=size_list,centers=center_list)

            objs_mask=rr['composite_mask']
            dimg=rr['img']
        
            nobjs0=nobjs
            for obj_idx in range(0,nobjs_cur):
                obj_mask=np.zeros(objs_mask.shape,dtype=np.uint8)
                obj_mask[objs_mask==obj_idx]=1
                ann=annotations_from_mask(obj_mask,obj_list[obj_idx]+1,nimgs+1,nobjs+1)
                #cv2.rectangle(dimg,ann['bbox'],(255,0,0), thickness=3)
                if(ann['area']>10):
                    annotations_list.append(ann)
                    nobjs+=1
            
            if nobjs==nobjs0:
                continue
            nimgs+=1
            outFileName='%06d.jpg'%nimgs
            images_list.append(get_image_info(dimg,nimgs,outFileName))
            cv2.imwrite(imageDir+outFileName,dimg)
            print((n+1,outFileName,obj_list,size_list))
            #cv2.imshow("dimg",dimg)
            #cv2.waitKey()

        data_coco = {}
        data_coco['images'] = images_list
        data_coco['categories'] = category_list
        data_coco['annotations'] = annotations_list

        class MyEncoder(json.JSONEncoder):
            def default(self, obj):
                if isinstance(obj, np.integer):
                    return int(obj)
                elif isinstance(obj, np.floating):
                    return float(obj)
                elif isinstance(obj, np.ndarray):
                    return obj.tolist()
                else:
                    return super(MyEncoder, self).default(obj)

        print(os.path.abspath('./'))
        json.dump(data_coco, open(annDir+setName+'.json', 'w'),cls=MyEncoder)

import glob
import fileinput
def getImageList(dir):
    flist=[]
    for f in glob.glob(dir+"/*.jpg"):
        flist.append(f)
    return flist

def readModelList(modelListFile):
    modelListDir=os.path.dirname(modelListFile)
    label_list=[]
    modelFiles=[]
    for line in fileinput.input(modelListFile):
        line=str(line)
        strs=line.split()
        label_list.append(strs[0])
        modelFiles.append(modelListDir+'/'+strs[1])
    return label_list,modelFiles

def main():
    dataDir='f:/home/aa/data/'
    imageFiles=getImageList(dataDir+'/VOCdevkit/VOC2012/JPEGImages/')

    modelListFile=dataDir+'/3dmodels/re3d2.txt'
    #modelListFile=dataDir+'/3dmodels/re3d25.txt'
    labelList,modelFiles=readModelList(modelListFile)

    outDir=dataDir+'3dgen/re3d2a/'

    dr=GenDet2dDataset(imageFiles, modelFiles, labelList)

    #gen eval set
    nImagesToGen=20
    maxModelsPerImage=2
    dr.gen(outDir,'eval',nImagesToGen,maxModelsPerImage)

    #gen train set
    nImagesToGen=100
    maxModelsPerImage=2
    dr.gen(outDir,'train',nImagesToGen,maxModelsPerImage)



if __name__=='__main__':
    main()




