import subprocess
import os

def compress_movie(input_path,output_path):
    #ffmpeg -i 输入.mp4 -s 800x480 -r 30 -c:v copy -c:a copy 输出.mp4
    print('开始压缩MP4')
    if not os.path.exists(input_path):
        return False
    try:
        cmd = [
            'ffmpeg',
            '-i', input_path,
            '-vf', 'scale=800:480',
            '-r', '30',
            '-c:v', 'libx264',
            '-profile:v', 'baseline',
            '-preset', 'fast',
            '-crf', '24',
            '-c:a', 'aac',
            '-b:a', '128k',
            '-y',
            output_path
        ]
        subprocess.run(cmd, check=True, capture_output=True, text=True, encoding='utf-8')
        print(f"视频转换成功: {input_path} -> {output_path}")
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