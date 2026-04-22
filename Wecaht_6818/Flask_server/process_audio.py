import subprocess
import os

def compress_audio(input_path,output_path):
    #ffmpeg -i 输入文件.m4a -c:a libmp3lame -qscale:a 2 输出文件.mp3
    print('开始压缩MP3')
    if not os.path.exists(input_path):
        return False
    try:
        cmd=[
            'ffmpeg',
            '-i',
            input_path,
            '-c:a',
            'libmp3lame',
            '-qscale:a',
            '2',
            output_path
        ]

        subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"音频转换成功: {input_path} -> {output_path}")
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