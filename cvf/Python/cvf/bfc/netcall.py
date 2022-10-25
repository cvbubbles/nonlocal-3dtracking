#from socketserver import TCPServer, StreamRequestHandler
import socketserver
import time
import numpy as np
import cv2
import struct

from numpy.core.defchararray import decode

BUFSIZ = 1024

'''
def encodeBytesList(bytesList):
    INT_SIZE=4
    totalSize=INT_SIZE*(len(bytesList)+1)
    head=struct.pack('!i',len(bytesList))
    for x in bytesList:
        head+=struct.pack('!i',len(x))
        totalSize+=len(x)
    
    #data=struct.pack('i',totalSize)
    data=head
    for x in bytesList:
        data+=bytes(x)
    return data

def decodeBytesList(data):
    INT_SIZE=4
    bytesList=[]
    data=bytes(data)
    count=struct.unpack('!i', data[0:INT_SIZE])[0]
    sizes=[]
    for i in range(1,count+1):
        t=data[i*INT_SIZE:i*INT_SIZE+INT_SIZE]
        isize=struct.unpack('!i', t)[0]
        sizes.append(isize)
    dpos=INT_SIZE*(count+1)
    for i in range(0,count):
        bytesList.append(data[dpos:dpos+sizes[i]])
        dpos+=sizes[i]
    return bytesList

def _decodeObj(data, typeLabel):
    obj=None
    if typeLabel=='image':
        nparr = np.frombuffer(bytes(data), np.uint8)
        obj = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
    return obj

def encodeObjs(objs):
    INT_SIZE=4
    subList=[[],[],[]]
    for k,v in objs.items():
        name,typeLabel=getNameType(v, k)
        subList[0].append(name.encode())
        subList[1].append(typeLabel.encode())
        subList[2].append(_encodeObj(v,typeLabel))

    totalSize=0
    for i in range(0,len(subList)):
        subList[i]=encodeBytesList(subList[i])
        totalSize+=len(subList[i])

    totalSize+=INT_SIZE*len(subList)
    data=struct.pack('!i',totalSize)
    for x in subList:
        data+=struct.pack('!i',len(x))
    
    for x in subList:
        data+=x
    return data
        
def decodeObjs(data):
    INT_SIZE=4
    subList=[0,0,0]
    for i in range(0,3):
        subList[i]=struct.unpack('!i', data[INT_SIZE*i:INT_SIZE*(i+1)])[0]
    p=INT_SIZE*len(subList)
    for i in range(0,3):
        isize=subList[i]
        subList[i]=decodeBytesList(data[p:p+isize])
        p+=isize
    objs=dict()
    nobjs=len(subList[0])
    for i in range(0,nobjs):
        name=subList[0][i].decode()
        type_=subList[1][i].decode()
        obj=_decodeObj(subList[2][i], type_)
        objs[name]=obj
    return objs
'''
def _encodeBytes(b):
    head=struct.pack('<i',len(b))
    return head+bytes(b)

def _decodeBytes(data, pos):
    size=struct.unpack('<i', data[pos:pos+4])[0]
    end=pos+4+size
    return data[pos+4:end],end

def getNameType(v,name):
    p=name.find(':')
    t=None
    if p<0:
        s=str(type(v))
        beg=s.find('\'')
        end=s.find('\'',beg+1)
        t=(name,s[beg+1:end])
    else:
        t=(name[0:p],name[p+1:])
    return t

def _packShape(shape):
    d=bytes()
    for i in shape:
        d+=struct.pack('<i',i)
    return d

def _encodeObj(v,typeLabel):
    rd=None
    if type(v)==np.ndarray:
        if typeLabel=='image':
            rd=cv2.imencode(".jpg", v)[1]
        else:
            rd=_packShape(v.shape)+v.tobytes()
    elif type(v)==str:
        rd=v.encode()
    elif type(v)==list:
        rd=_packShape([len(v)])
        if len(v)>0:
            dtype=type(v[0])
            withElemSize= not (dtype==int or dtype==float)
            for x in v:
                if type(x)!=dtype:
                    raise 'list elems must have the same type'
                xb=_encodeObj(x,typeLabel)
                if withElemSize:
                    rd+=_encodeBytes(xb)
                else:
                    rd+=xb
    elif type(v)==int:
        rd=struct.pack('<i',v)
    elif type(v)==float:
        rd=struct.pack('<f',v)
    else:
        raise 'unknown type'
        
    return rd

def encodeObjs(objs):
    INT_SIZE=4
    data=bytes()
    for k,v in objs.items():
        name,typeLabel=getNameType(v, k)
        data+=_encodeBytes(name.encode())
       #data+=_encodeBytes(typeLabel.encode())
        objBytes=_encodeObj(v,typeLabel)
        data+=_encodeBytes(objBytes)

    totalSize=len(data)
    head=struct.pack('<i',totalSize)

    return head+data
'''
class  BytesObject:
    tConfig={
        'char':('<c',1),
        'int':('<i',4),
        'float':('<f',4),
        'double':('<d',8)
    }

    def __init__(self, data):
        self.data=bytes(data)

    def decode_as(self,typeLabel):
        INT_SIZE=4
        obj=None
        data=self.data
        if typeLabel=='image':
            nparr = np.frombuffer(data, np.uint8)
            obj = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        elif typeLabel=='str':
            n=struct.unpack_from('<i',data,0)[0]
            assert len(data)==n+INT_SIZE
            obj=data[INT_SIZE:].decode()
        else:
            if typeLabel not in BytesObject.tConfig:
                raise 'unknown type'
            
            cfg=BytesObject.tConfig[typeLabel]
            SIZE=cfg[1]
            fmt=cfg[0]
            data=self.data
            
            if len(data)==SIZE:
                obj=struct.unpack_from(fmt,data,0)[0]
            else:
                n=struct.unpack_from('<i',data,0)[0]
                if len(data)==SIZE*n+INT_SIZE:
                    obj=[]
                    for i in range(0,n):
                        v=struct.unpack_from(fmt,self.data,i*SIZE+INT_SIZE)[0]
                        obj.append(v)
        return obj
'''

class  BytesObject:
    tConfig={
        'char':(np.char,1,'<c'),
        'int':(np.int,4,'<i'),
        'float':(np.float32,4,'<f'),
        'double':(np.float64,8,'<d')
    }

    def __init__(self, data):
        self.data=bytes(data)
        
    def decode_list(self, typeLabel):
        data=self.data
        INT_SIZE=4
        withElemSize=typeLabel not in BytesObject.tConfig

        size=struct.unpack_from('<i',data,0)[0]
        p=INT_SIZE
        dl=[]
        if withElemSize:
            for i in range(0,size):
                isize=struct.unpack_from('<i',data,p)[0]
                p=p+INT_SIZE
                v=self.decode_(data[p:p+isize], typeLabel)
                p=p+isize
                dl.append(v)
        else:
            cfg=BytesObject.tConfig[typeLabel]
            elemSize=cfg[1]
            fmt=cfg[2]
            for i in range(0,size):
                v=struct.unpack_from(fmt,data,p)[0]
                p+=elemSize
                dl.append(v)
        return dl

    def decode(self, typeLabel):
        return self.decode_(self.data,typeLabel)

    def decode_(self,data,typeLabel):
        obj=None
        INT_SIZE=4
        readSize=0
        #data=self.data
        if typeLabel=='image':
            #n=struct.unpack_from('<i',data,0)[0]
            #assert len(data)>=n+INT_SIZE
            nparr = np.frombuffer(data, np.uint8, offset=0)
            obj = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            #readSize=n+INT_SIZE
        elif typeLabel=='str':
            #n=struct.unpack_from('<i',data,0)[0]
            #assert len(data)==n+INT_SIZE
            obj=data.decode()
            #readSize=n+INT_SIZE
        else:
            if typeLabel not in BytesObject.tConfig:
                raise 'unknown type'
            
            cfg=BytesObject.tConfig[typeLabel]
            dtype_=cfg[0]
            SIZE=cfg[1]
            fmt=cfg[2]
            #data=self.data
            
            # if len(data)==SIZE:
            #     obj=struct.unpack_from(fmt,data,0)[0]
            # else:
            shape=[]
            if len(data)>SIZE:
                n=1
                totalSize=0
                for i in range(0,4):
                    m=struct.unpack_from('<i',data,i*INT_SIZE)[0]
                    shape.append(m)
                    n=n*m
                    totalSize=n*SIZE+(i+1)*INT_SIZE
                    if totalSize>=len(data):
                        break
                if totalSize>len(data):
                    raise 'invalid size'
            else:
                assert len(data)==SIZE

            dim=len(shape)
            arr=np.frombuffer(data,offset=dim*INT_SIZE,dtype=dtype_)
            if dim==0:
                obj=dtype_(arr[0])
            #    print(type(obj))
            else:
                obj=np.reshape(arr,tuple(shape))

        return obj


def decodeObjs(data):
    INT_SIZE=4
    p=0
    dsize=len(data)
    objs=dict()
    while p<dsize:
        objBytes,p=_decodeBytes(data, p)
        name=objBytes.decode()
        #objBytes,p=_decodeBytes(data, p)
        #type_=objBytes.decode()
        objBytes,p=_decodeBytes(data, p)
        #obj=_decodeObj(objBytes, type_)
        objs[name]=BytesObject(objBytes)
    return objs


def recvObjs(rq):
    INT_SIZE=4
    buf=rq.recv(INT_SIZE)
    if len(buf)<INT_SIZE:
        return None
    #if len(size)!=INT_SIZE:
    assert len(buf)==INT_SIZE
    totalSize=struct.unpack('<i',bytes(buf))[0]
    data=bytes()
    while len(data)<totalSize:
        buf=rq.recv(BUFSIZ)
        data+=bytes(buf)
        #print(len(data))
    return decodeObjs(data)



def runServer(address, handleFunc):
    #address = ('127.0.0.1', 8000)

    class NetcallRequestHandler(socketserver.BaseRequestHandler):
        # 重写 handle 方法，该方法在父类中什么都不做
        # 当客户端主动连接服务器成功后，自动运行此方法
        def handle(self):
            # client_address 属性的值为客户端的主机端口元组
            print('... connected from {}'.format(self.client_address))

            while True:
                try:
                    objs=recvObjs(self.request) 
                    if objs==None:
                        break

                    cmd=objs['cmd'].decode('str')
                    if cmd=='exit':
                        print('...disconnet from {}'.format(self.client_address))
                        break

                    rdata=handleFunc(objs)

                    self.request.send(rdata) 
                except:
                    dobjs={'error':-1}
                    rdata=encodeObjs(dobjs)
                    self.request.send(rdata)

    tcp_server = socketserver.TCPServer(address, NetcallRequestHandler)
    print('等待客户端连接...')
    try:
        tcp_server.serve_forever()  # 服务器永远等待客户端的连接
    except KeyboardInterrupt:
        tcp_server.server_close()   # 关闭服务器套接字
        print('\nClose')
        exit()


# 创建 StreamRequestHandler 类的子类
class MyRequestHandler(socketserver.BaseRequestHandler):
    # 重写 handle 方法，该方法在父类中什么都不做
    # 当客户端主动连接服务器成功后，自动运行此方法
    def handle(self):
        # client_address 属性的值为客户端的主机端口元组
        print('... connected from {}'.format(self.client_address))

        objs=recvObjs(self.request)       
        # dimg=objs['img'].decode_as('image')
        # cv2.imwrite('/home/fan/dimg.jpg',dimg)
        x=objs['x'].decode('int')
        y=objs['y'].decode('int')
        z=objs['z'].decode('str')
        img=objs['img'].decode('image')
        vx=objs['vx'].decode_list('str')

        print(x)
        print(y)
        print(z)
        print(vx)

        dobjs={
            'x':x,'y':y,'z':z,'img:image':img, 'vx':vx
        }
        data=encodeObjs(dobjs)

        self.request.send(data)

import sys
def test_encode():
    objs={
        'x':1.0, 'y':[1,2,3],'z':['he','she','me']
    }
    
    data=encodeObjs(objs)

    objs=decodeObjs(data[4:])
    x=objs['x'].decode('float')
    y=objs['y'].decode_list('int')
    z=objs['z'].decode_list('str')

    data=data

def main():
    #test_encode()
    #return

    # 创建 TCP 服务器并启动，该服务器为单线程设计，不可同时为两个客户端收发消息
    # 该服务器每次连接客户端成功后，运行一次 handle 方法然后断开连接
    # 也就是说，每次客户端的请求就是一次收发消息
    # 连续请求需要客户端套接字不断重新创建

    ADDR = ('127.0.0.1', 8000)

    tcp_server = socketserver.TCPServer(ADDR, MyRequestHandler)
    print('等待客户端连接...')
    try:
        tcp_server.serve_forever()  # 服务器永远等待客户端的连接
    except KeyboardInterrupt:
        tcp_server.server_close()   # 关闭服务器套接字
        print('\nClose')
        exit()


if __name__ == '__main__':
    main()