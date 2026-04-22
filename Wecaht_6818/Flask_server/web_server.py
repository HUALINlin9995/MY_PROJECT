from flask import Flask,jsonify,request,send_from_directory
from flask_cors import CORS
import os
import time

import process_movie
import proess_img
import process_audio
import call_AI
import logging

#关闭日志
logging.getLogger('werkzeug').setLevel(logging.ERROR)

app = Flask(__name__,static_folder='.')
CORS(app)
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
USER_DATA = os.path.join(BASE_DIR, "user_data")
# IP='https://1nd2256xr2130.vicp.fun'
IP='http://127.0.0.1:5000'
# IP='http://10.206.245.88:5000'

#发送开发板的临时消息
Temporary_Message={"operation":"","url":"","name":[],"mode":"","time":""}
#发送小程序的临时消息
wx_Message={"msg":""}
is_error="正常"

#获取客户端发送的图片视频音频
def get_image_list(is_img):
    file=request.files['file']
    global Temporary_Message
    #获取当前时间
    time_str=time.strftime("%Y%m%d_%H%M%S",time.localtime())
    #处理图片
    if is_img==1:
        print('开始压缩')
        file.save(f'./user_data/user_img/{time_str}.jpg')
        #启动压缩程序压缩成bmp文件
        if proess_img.compress_image(f'./user_data/user_img/{time_str}.jpg',f'./user_data/user_img/{time_str}.bmp'):
            print('压缩成功')
        #存入临时消息内
        #这里输本机与开发板连接的ip，非wlan的ip
        Temporary_Message["operation"]="add_img"
        print(f"http://192.168.1.100:5000/user_data/user_img/{time_str}.bmp")
        Temporary_Message["url"]=f"http://192.168.1.100:5000/user_data/user_img/{time_str}.bmp"
        return jsonify({
            "code":200,
            "msg":"图片接收成功"
         })

    #处理视频
    elif is_img==0:
        file.save(f'./user_data/user_movie/1.mp4')
        if process_movie.compress_movie('./user_data/user_movie/1.mp4',f'./user_data/user_movie/{time_str}.mp4'):
            print('压缩成功')

        Temporary_Message["operation"]="add_movie"
        print(f"http://192.168.1.100:5000/user_data/user_movie/{time_str}.mp4")
        Temporary_Message["url"]=f"http://192.168.1.100:5000/user_data/user_movie/{time_str}.mp4"

        return jsonify({
            "code": 200,
            "msg": "视频接收成功"
        })

    #处理音频
    elif is_img==2:
        audio_file = request.files['file']
        original_filename = audio_file.filename
        #先保存，因为不知道是不是mp3文件
        save_path = f'./user_data/user_audio/{original_filename}'
        audio_file.save(save_path)
        ext = os.path.splitext(original_filename)[1].lower()
        #若为mp3则不用处理
        if ext=='.mp3':
            audio_file.save(f'./user_data/user_audio/{time_str}.mp3')
            os.remove(save_path)
        else:
            if process_audio.compress_audio(save_path,f'./user_data/user_audio/{time_str}.mp3'):
                print('已压缩文件')

        Temporary_Message["operation"]="add_audio"
        print(f"http://192.168.1.100:5000/user_data/user_audio/{time_str}.mp3")
        Temporary_Message["url"]=f"http://192.168.1.100:5000/user_data/user_audio/{time_str}.mp3"

        return jsonify({
            "code": 200,
            "msg": "音频接收成功"
        })
    else:
        return jsonify({"code": 400, "msg": f"无效参数: is_img={is_img}"})

#获取视频音频图片
def get_user_data(name):
    data_dir=os.path.join(USER_DATA,name)
    data_list=[]
    for filename in os.listdir(data_dir):
        # 拼接成客户端能直接访问的URL
        url = f"{IP}/user_data/{name}/{filename}"
        data_list.append(url)
    return jsonify({
        "code": 200,
        "data": data_list
    })

#删除视频音频
def delete_data(name):
    global Temporary_Message
    data=request.get_json()
    http=data['http']
    print('收到删除请求:',http)
    filename = os.path.basename(http)

    Temporary_Message = {"operation": "", "url": "", "name": [], "mode": "", "time": ""}
    Temporary_Message["operation"] = "del_movie" if name == "user_movie" else "del_audio"
    Temporary_Message["name"] = [filename]
    print(Temporary_Message)

    file_path=os.path.join(USER_DATA,name,filename)
    if not os.path.exists(file_path):
        return jsonify({"code": 404, "msg": "文件不存在"}), 404
    #因为小程序播放会持续占用资源，所以一直循环直到删除
    while True:
        try:
            os.remove(file_path)
            print("删除成功！")
            break
        except PermissionError:
            time.sleep(0.1)
    return jsonify({"code": 200, "msg": "删除成功"})

#--------------------------业务区-------------------------
#主页面，用于测试http
@app.route('/')
def home():
    return '这是华林的小程序服务器'

#托管整个文件夹
@app.route('/user_data/<path:filepath>')
def serve_user_data(filepath):
    return send_from_directory(USER_DATA, filepath)

#获取轮播图
@app.route('/carousel',methods=['GET'])
def get_user_img():
    return get_user_data('carousel')

#获取壁纸
@app.route('/user_img',methods=['GET'])
def get_carousel():
    return get_user_data('user_img')

#获取视频
@app.route('/user_movie',methods=['GET'])
def get_movie():
    return get_user_data('user_movie')

#获取音频
@app.route('/user_audio',methods=['GET'])
def get_audio():
    return get_user_data('user_audio')


#-------修改数据--------


#删除图片
@app.route('/delete/img',methods=['POST'])
def delete_img():
    global Temporary_Message
    data=request.get_json()
    http_list=data['http']
    print('收到删除请求:',http_list)

    del_img=[]

    #遍历列表批量删除
    for http in http_list:
        filename = os.path.basename(http)
        del_img.append(filename)
        file_path=os.path.join(USER_DATA,'user_img',filename)
        if not os.path.exists(file_path):
            return jsonify({"code": 404, "msg": "文件不存在"}), 404
        while True:
            try:
                os.remove(file_path)
                print("删除成功！")
                break
            except PermissionError:
                time.sleep(0.1)

    Temporary_Message = {"operation": "", "url": "", "name": [], "mode": "", "time": ""}
    Temporary_Message["operation"] = "del_img"
    Temporary_Message["name"] = del_img
    print(Temporary_Message)

    return jsonify({"code": 200, "msg": "删除成功"})

#删除视频
@app.route('/delete/movie',methods=['POST'])
def delete_movie():
    return delete_data('user_movie')

#删除音频
@app.route('/delete/audio',methods=['POST'])
def delete_audio():
    return delete_data('user_audio')

#接收客户端的壁纸
@app.route('/api/upload',methods=['POST'])
def upload():
    return get_image_list(1)

#接收客户端的视频
@app.route('/api/mp4',methods=['POST'])
def mp4():
    return get_image_list(0)

#接收客户端的音频
@app.route('/api/audio',methods=['POST'])
def mp3():
    return get_image_list(2)

#输入命令
@app.route('/api/input_cmd',methods=['POST'])
def input_cmd():
    global Temporary_Message
    data=request.get_json()
    print('收到消息:',data)
    Temporary_Message = {"operation": "", "url": "", "name": [], "mode": "", "time": ""}
    Temporary_Message["operation"] = "cmd"
    Temporary_Message["url"]=data["cmd"]
    return jsonify({"code":200,"msg":"收到命令"})

#调用ai大模型
@app.route('/api/audio_AI',methods=['POST'])
def audio_AI():
    global Temporary_Message
    data=request.get_json()
    print('收到消息:',data['text'])
    cmd=call_AI.get_dev_board_command(data['text'])
    print(cmd)
    Temporary_Message = {"operation": "", "url": "", "name": [], "mode": "", "time": ""}
    Temporary_Message["operation"] = "cmd"
    Temporary_Message["url"]=cmd
    return jsonify({"code":200,"msg":"收到命令"})

#获取开发板cmd返回
@app.route('/api/output_cmd/',methods=['POST'])
def output_cmd():
    global wx_Message
    global is_error
    data = request.get_json()
    print('收到消息:',data['data'])
    if data['data']=='error':
        is_error='报警'
    wx_Message['msg']=data['data']
    return jsonify({"code":200,"msg":"收到命令"})


#--------------监听区-------------

#这里是专用于开发板实时监听的api
@app.route('/Temporary_message',methods=['GET'])
def ls_message():
    global Temporary_Message
    message=Temporary_Message["operation"]
    url=Temporary_Message["url"]
    name=Temporary_Message['name']
    mode = Temporary_Message['mode']
    time = Temporary_Message['time']
    Temporary_Message = {"operation": "", "url": "","name":"","time":"","mode":""}
    if message and url and name:
        print(f"发送给开发板处理逻辑:[{message}],url为[{url}],name为[{name}]")
    if message == 'set_movie':
        return jsonify({
            "code":200,
            "msg":message,
            "url":url,
            "mode":mode,
            "time":time
        })
    return jsonify({
        "code":200,
        "msg":message,
        "url":url,
        "name":name
    })

#专用小程序的api
@app.route('/wx_msg',methods=['GET'])
def ls_wxmsg():
    global wx_Message
    data=wx_Message['msg']
    if data:
        print('发送给小程序',data)
    wx_Message={'msg':""}
    return jsonify({
        "code":200,
        "msg":data
    })


#--------------测试区-------------
@app.route('/api/message',methods=['GET'])
def message_user():
    return jsonify({"code":200,"msg":""})

#小程序持续监听接口
@app.route('/user_alarm',methods=['GET'])
def user_alarm():
    global is_error
    if is_error=='报警':
        is_error = '正常'
        return jsonify({"code": 200, "msg": "报警"})
    return jsonify({"code":200,"msg":"正常"})

#接收设置壁纸播放视频音频
@app.route('/remote/play',methods=['POST'])
def play_video():
    global Temporary_Message
    data=request.get_json()
    filename=os.path.basename(data['Url'])
    Temporary_Message = {"operation": "", "url": "", "name": [], "mode": "", "time": ""}
    print(data)
    print(filename)
    Temporary_Message['url'] = filename
    if data['msg']=='wallpaper':
        print("收到设置图片请求:",filename)
        Temporary_Message['operation']='set_wallpaper'
    elif data['msg']=='movie':
        print("收到播放视频请求：",filename)
        Temporary_Message['operation']='set_movie'
        Temporary_Message['mode']=data['playMode']
        Temporary_Message['time']=data['playTime']
    elif data['msg']=='audio':
        print("收到播放音频请求：", filename)
        Temporary_Message['operation'] = 'set_audio'
    print(Temporary_Message)
    return jsonify({"code":200,"msg":"收到播放请求"})



if __name__ == '__main__':
    app.run(host='0.0.0.0',port=5000,debug=True)