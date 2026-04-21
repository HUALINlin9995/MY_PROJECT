import subprocess
import os

def compress_image(input_path, output_path):
    if not os.path.exists(input_path):
        return False
    try:
        # 构建 ffmpeg 命令
        cmd = [
            'ffmpeg',
            '-i', input_path,
            '-s', '800x480',  # 设置分辨率为 800*480
            '-c:v', 'bmp',     # 设置输出格式为 bmp
            '-y',              # 覆盖已存在的文件
            output_path
        ]

        subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"图片压缩成功: {input_path} -> {output_path}")
        if os.path.exists(input_path):
            os.remove(input_path)
            print("已删除原始文件：", input_path)
        return True
    except subprocess.CalledProcessError as e:
        print(f"压缩失败: {e.stderr}")
        return False
    except Exception as e:
        print(f"发生错误: {str(e)}")
        return False


