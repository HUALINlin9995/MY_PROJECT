
// app.js
App({
  onLaunch() {
    // 展示本地存储能力
    const logs = wx.getStorageSync('logs') || []
    logs.unshift(Date.now())
    wx.setStorageSync('logs', logs)

    // 登录
    wx.login({
      success: res => {
        // 发送 res.code 到后台换取 openId, sessionKey, unionId
      }
    })
  },
  showNowTime(){
    var date = new Date();
    var month = (date.getMonth() + 1);
    var day = date.getDate();
    var hour = date.getHours();
    var minute = date.getMinutes();
    var currentTime = `${month}月${day}日  ${hour}:${minute}`;
    return currentTime
  },
  globalData: {
    userInfo: null,
    globalShowAlert: false,
    globalList:[]
  },

  get_alarm(){
    console.log('开始监听报警')
    wx.request({
        url:'http://127.0.0.1:5000/user_alarm',
        method:'GET',
        timeout:10000,
        success:(res)=>{
          if(res.data.msg==='报警')
          {
            wx.showToast({
              'title':'开发板报警提示',
              icon:'error',
              duration:5000
            }),
            console.log("收到报警")
            this.globalData.globalShowAlert=true
            this.globalData.globalList.push(`!${this.showNowTime()}    开发板报警提示`)
          }
          else if(res.data.msg==='回复')
          {
            this.globalData.globalList.push(`${this.showNowTime()}    开发板执行语音命令成功`)
          }
        },
        fail:(err)=>{

        }
    })
  },

  onLoad(options) {
    this.get_alarm()
  }

})
