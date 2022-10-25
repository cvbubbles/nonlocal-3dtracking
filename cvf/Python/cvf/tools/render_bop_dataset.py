
import os
import sys

#envs=os.environ.get("PATH")
#os.environ['PATH']=envs+';F:/dev/cvfx/assim410/bin-v140/x64/release/;F:/dev/cvfx/opencv3413/bin-v140/x64/Release/;F:/dev/cvfx/bin/x64/;D:/setup/Anaconda3/;'

import cv2
from cv2 import data
from skimage import measure

import random
import numpy as np

def categories(label, label_id):
    category = {}
    category['supercategory'] = 'component'
    category['id'] = label_id
    category['name'] = label
    return category

def get_category_list(label_list):
    category_list = []
    for i,label in enumerate(label_list):
        category_list.append(categories(label, i+1))
    return category_list


def rand_box(imsize, minSize, maxSize):
    x = 0
    y = 0
    while True:
        #x=random.randint(0,imsize[0])
        #y=random.randint(0,imsize[1])
        x = int(random.uniform(0, imsize[0]))
        y = int(random.uniform(0, imsize[1]))
        if min(x, y) > minSize:
            break
    size = random.randint(minSize, min(x, y, maxSize))
    return [x-size, y-size, size]


import cvf.cvrender as cvr
import json

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

class GenBOPDataset:
    def __init__(self, outDir, imageFiles, modelFiles, labelList) -> None:
        self.dr = cvr.DatasetRender()
        self.dr.loadModels(modelFiles)

        self.imageFiles = imageFiles
        self.modelFiles = modelFiles
        self.category_list = get_category_list(labelList)
        self.labelList = labelList
        self.outDir = outDir

    def saveBOPModels(self):
        modelDir = self.outDir + '/models/'
        os.makedirs(modelDir, exist_ok=True)
        models = self.dr.getModels()
        nModels = len(models)
        
        for i in range(0, nModels):
            fname = 'obj_%06d.ply'%(i+1)
            print(fname, ':', self.modelFiles[i])
            models[i].saveAs(modelDir+fname)

        modelsInfo=self.dr.getBOPModelInfo()
        json.dump(modelsInfo, open(modelDir+'models_info.json', 'w'), cls=MyEncoder)

    def genScene(self, setName, sceneId, nImages, imgSize, maxModelsPerImage, minModelsPerImage, maxObjectSizeRatio=0.5, minObjectSizeRatio=0.2):
        sceneDir=self.outDir+'/'+setName+'/'+'%06d'%sceneId+'/'
        imageDir=sceneDir+'rgb/'
        maskDir=sceneDir+'mask/'
        maskVisibDir=sceneDir+'mask_visib/'
        os.makedirs(imageDir,exist_ok=True)
        os.makedirs(maskDir,exist_ok=True)
        os.makedirs(maskVisibDir,exist_ok=True)

        category_list=self.category_list
        
        scene_gt={}
        scene_gt_info={}
        scene_camera={}

        #idx_of_all_models=list(range(len(label_list)))
        n_all_models=len(self.labelList)
        max_models_perim=min(n_all_models,maxModelsPerImage)
        min_models_perim=min(minModelsPerImage,max_models_perim)
        min_object_size=int(min(imgSize)*minObjectSizeRatio)
        max_object_size=int(min(imgSize)*maxObjectSizeRatio)

        nobjs=0
        idList=[i for i in range(0,n_all_models)]
        for n in range(0,nImages):
            img=cv2.imread(self.imageFiles[random.randint(0,len(self.imageFiles)-1)])
            if img is None:
                continue
            dsize=tuple(imgSize)
            img=cv2.resize(img,dsize)

            obj_list=[]
            size_list=[]
            center_list=[]
            nobjs_cur=random.randint(min_models_perim,max_models_perim)
            random.shuffle(idList)
            for i in range(0,nobjs_cur):
                obj_list.append(idList[i])
                size=int(random.uniform(min_object_size,max_object_size))

               # imaxSize=min(int(min(dsize)*4/5),400)
               # size=int(random.uniform(150,imaxSize))

                rbb=rand_box(dsize,size,size+1)
                size_list.append(rbb[2])
                center_list.append([int(rbb[0]+rbb[2]/2),int(rbb[1]+rbb[2]/2)])
        
            rr=self.dr.renderToImage(img,obj_list,sizes=size_list,centers=center_list)

            imKey='%d'%(n+1)
            scene_gt_i=[]
            
            vR=rr['vR']
            vT=rr['vT']
            #x=np.reshape(vR[0],9)
            for i in range(0,nobjs_cur):
                scene_gt_i.append(
                    {'cam_R_m2c':np.reshape(vR[i],9),
                    'cam_t_m2c':vT[i],
                    'obj_id':obj_list[i]+1
                    }
                )
            scene_gt[imKey]=scene_gt_i
            scene_gt_info[imKey]=rr['bop_info']
            scene_camera[imKey]={
                'cam_K':np.reshape(rr['K'],9),
                'depth_scale':0.1
            }

            dimg=rr['img']
            outFileName='%06d.png'%(n+1)
          
            cv2.imwrite(imageDir+outFileName,dimg)

            masks=rr['objs_mask']
            masks_visib=rr['objs_mask_visib']
            for i in range(0,nobjs_cur):
                outFileName='%06d'%(n+1)+'_%06d.png'%i
                cv2.imwrite(maskDir+outFileName,masks[i])
                cv2.imwrite(maskVisibDir+outFileName,masks_visib[i])

            print((n+1,outFileName,obj_list,size_list))
            #cv2.imshow("dimg",dimg)
            #cv2.waitKey()

        print(os.path.abspath('./'))
        json.dump(scene_gt, open(sceneDir+'scene_gt.json', 'w'), cls=MyEncoder)
        json.dump(scene_gt_info, open(sceneDir+'scene_gt_info.json', 'w'), cls=MyEncoder)
        json.dump(scene_camera, open(sceneDir+'scene_camera.json', 'w'), cls=MyEncoder)

        camK=scene_camera['1']['cam_K']
        camInfo={
            'cx':camK[2],
            'cy':camK[5],
            'depth_scale':1.0,
            'fx':camK[0],
            'fy':camK[4],
            'height':imgSize[1],
            'width':imgSize[0]
        }
        json.dump(camInfo, open(self.outDir+'/camera.json', 'w'), cls=MyEncoder)


import glob
import fileinput
def getImageList(dir):
    """
    get the absolute paths of all the file which suffix is like .jpg in dir root
    """
    flist = []
    for f in glob.glob(dir+"/*.jpg"):
        flist.append(f)
    return flist

def readModelList(modelListFile):
    """
    get modelListFile's content
    and return the relative model's lable and model's absolute path
    """
    modelListDir = os.path.dirname(modelListFile)  # get the parent path
    label_list = []
    modelFiles = []
    for line in fileinput.input(modelListFile):
        line = str(line)
        strs = line.split()
        label_list.append(strs[0])
        modelFiles.append(modelListDir+'/'+strs[1])
    return label_list, modelFiles

def main():
    dataDir = '/home/aa/data/'
    imageFiles = getImageList(dataDir+'/VOCdevkit/VOC2012/JPEGImages/')

    #modelListFile = dataDir+'/3dmodels/re3d3.txt'
    #modelListFile=dataDir+'/3dmodels/re3d25.txt'
    modelListFile=dataDir+'/3dmodels/ycbv.txt'
    labelList, modelFiles = readModelList(modelListFile)

    outDir = dataDir + '3dgen/ycbv_bop21/'  # save path for generate BOP type data

    dr = GenBOPDataset(outDir, imageFiles, modelFiles, labelList)
    dr.saveBOPModels()

    #gen eval set
    setName = 'train'
    nScenesToGen = 12
    nImagesPerScene = 1000
    maxModelsPerImage = 15
    #dimgSize = (800,600)
    dimgSize = (640,480)

    for sceneId in range(0, nScenesToGen):
        dr.genScene(setName, sceneId+1, nImagesPerScene, dimgSize, maxModelsPerImage=maxModelsPerImage, minModelsPerImage=8)



if __name__=='__main__':
    main()




