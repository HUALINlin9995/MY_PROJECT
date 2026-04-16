import requests
from bs4 import BeautifulSoup
import re
import csv
import threading
from queue import Queue

def requests_url(data):
    soup=BeautifulSoup(data.text,'html.parser')
    find_tryurl=soup.find_all('a',class_='truncate')
    this_urls=[]
    this_name=[]
    for item in find_tryurl:
        find_url=item.get('href')
        find_url='https://liaoxuefeng.com'+find_url
        find_tag=re.findall('<span class="gsc-index-item-marker">(.*?)</span>',str(item))
        find_name=re.findall('<span class="gsc-index-item-title">(.*?)</span>',str(item))
        if len(find_url)>23:
            this_urls.append(find_url)
        this_name.append(find_tag[0]+find_name[0])
    this_httpd=dict(zip(this_name,this_urls))
    return this_httpd

def requests_text(url):
    try:
        data=requests.get(url, timeout=10)
        data.encoding=data.apparent_encoding
        soup=BeautifulSoup(data.text,'html.parser')
        intro=soup.find('div',id='gsi-chapter-content')
        if intro:
            intro_text=intro.get_text(strip=True,separator='\n')
            return intro_text
    except Exception as e:
        print(f"爬取出错: {url}, 错误: {e}")
    return ''

# 多线程
url_queue = Queue()    # 任务队列
result_dict = {}       # 存储爬取结果
lock = threading.Lock()# 线程锁

def worker():
    global result_dict
    while not url_queue.empty():
        title, url = url_queue.get()
        content = requests_text(url)
        with lock:
            result_dict[title] = content
            print(f"爬取完成: {title}")
        url_queue.task_done()

def requests_textall_multi(the_httpd):
    global result_dict
    result_dict.clear()

    # 把所有章节放入队列
    for title, url in the_httpd.items():
        url_queue.put((title, url))

    # 启动 5 个线程
    thread_num = 5
    threads = []
    for i in range(thread_num):
        t = threading.Thread(target=worker)
        threads.append(t)
        t.start()

    print(f"已启动 {thread_num} 个线程同时爬取...")
    url_queue.join()  # 等待所有任务完成

    # 写入 CSV
    headerss = ['目录', '内容']
    with open('thedata.csv', 'w', encoding='utf-8-sig', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(headerss)
        for cat, datas in result_dict.items():
            writer.writerow([cat, datas])

    print('所有内容爬取完毕，已保存到 thedata.csv')

def read_csv(filenames):
    csv_user_data={}
    with open(filenames,'r',encoding='utf-8-sig')as f:
        thedata=csv.reader(f)
        next(thedata)
        for i in thedata:
            if len(i)>=2:
                csv_user_data[i[0].strip()]=i[1].strip()
            else:
                continue
    return csv_user_data

if __name__ == '__main__':
    url='https://liaoxuefeng.com/books/python/introduction/index.html'
    data=requests.get(url)
    data.encoding=data.apparent_encoding
    print('正在爬取目录')
    the_httpd=requests_url(data)
    print('爬取成功啦！')

    csv_httpd=[[cat,url]for cat,url in the_httpd.items()]
    headers=['目录','网址']
    with open('thehttpd.csv','w',encoding='utf-8-sig',newline='')as f:
        writer = csv.writer(f)
        writer.writerow(headers)
        writer.writerows(csv_httpd)

    while True:
        print('*'*30,'1.查看目录','*'*30)
        print('*'*30,'2.查看目录文本','*'*30)
        print('*'*30,'3.爬取所有文本','*'*30)
        print('*'*30,'4.读取爬取后的所有文本','*'*30)
        print('*'*30,'5.退出系统','*'*30)
        user_choose=input('请输入需要执行的操作(数字输入):')
        if not user_choose.isdigit():
            print('输入错误，请重新输入')
            continue
        user_choose=int(user_choose)

        if user_choose==1:
            for i in the_httpd:
                print(i)
            continue

        if user_choose==2:
            while True:
                user_choosed = input('请选择想要查看的章节(输入esc退出):')
                if user_choosed=='esc':
                    break
                if user_choosed not in the_httpd:
                    print('未找到该章节，请重新输入')
                    continue
                user_choosed_data=requests_text(the_httpd[user_choosed])
                print(user_choosed_data)

        if user_choose==3:
            requests_textall_multi(the_httpd)  # 调用多线程版本
            continue

        if user_choose==4:
            user_choose_datas=read_csv('thedata.csv')
            for i in user_choose_datas:
                print('-'*50,i,'-'*50)
                print(user_choose_datas[i])
            continue

        if user_choose==5:
            print('退出成功')
            break

        else:
            print('输入错误，请重新输入')
