import os

mlx_path = "/home/aa/libs/cvf/Python/compress/compressmodel.mlx"  # mlx脚本的路径
root_dir = "/home/aa/data/3dmodels"  # 需要处理模型的根路径


for dr in os.listdir(root_dir):
    model_dir = os.path.join(root_dir, dr)
    if os.path.isdir(model_dir):
        model_path = os.path.join(model_dir, dr+".3ds")  # 模型的路径，模型后缀根据实际修改
        print("Current process model: ",model_path)
        output_path = os.path.join(model_dir, dr+".ply")  # 导出模型的路径
        cmd = "meshlabserver -i " + model_path + " -o " + output_path + " -s " + mlx_path + " -om fc fn wt"  # 需要执行的命令
        # -om 参数根据导出需求自行修改
        os.system(cmd)
        
