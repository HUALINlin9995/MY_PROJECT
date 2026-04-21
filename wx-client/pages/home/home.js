// pages/home/home.js
// const { noneParamsEaseFuncs } = require('XrFrame/xrFrameSystem');
const config = require('../../config/index.js')
const plugin = requirePlugin("WechatSI");
const manager = plugin.getRecordRecognitionManager();
const app = getApp()
Page({

  /**
   * 页面的初始数据
   */
  data: {
    swiperList: [],
    selectedIndex: -1,
    text: "",
    showTextDisplay: false
  },

  getSwiperList() {
    console.log("开始连接服务器")
    wx.request({
      url: config.SERVER_IP + '/carousel',
      method: 'GET',
      timeout: 10000,
      success: (res) => {
        console.log("服务器响应成功", res)
        if (res.statusCode === 200) {
          this.setData({
            swiperList: res.data.data
          })
          wx.stopPullDownRefresh()
          //存入全局消息列表
          var nowTime=app.showNowTime()
          app.globalData.globalList.push(`${nowTime}    服务器连接成功`);
        } else {
          wx.showToast({
            title: '服务器异常',
            icon: 'none',
            duration: 2000
          })
        }
      },
      fail: (err) => {
        console.log("连接服务器失败", err)
        wx.showToast({
          title: '无法连接服务器，请检查网络',
          icon: 'none',
          duration: 5000
        })
        wx.stopPullDownRefresh()
        this.setData({
          swiperList: ['../img/error.jpg']
        })
      }
    })
  },

  wallpaper_button() {
    wx.navigateTo({
      url: '../open_wallpaper/open_wallpaper'
    });
  },

  video_button() {
    wx.navigateTo({
      url: '../open_video/open_video'
    });
  },

  audio_button() {
    wx.navigateTo({
      url: '../open_audio/open_audio'
    })
  },

  selectImage(e) {
    const index = e.currentTarget.dataset.index;
    this.setData({
      selectedIndex: index
    });
  },

  startRecord() {
    console.log('长按逻辑')
    this.setData({
      showTextDisplay: true
    });
    manager.start({
      lang: "zh_CN"
    });
    wx.showLoading({
      title: '正在收听',
      mask: false
    })
  },

  stopRecord() {
    console.log('录音完成')
    manager.stop();
    console.log('语音转换', this.text)
  },

  //发送同声传译数据
  send_text() {
    wx.request({
      url: config.SERVER_IP + '/api/audio_AI',
      method: "POST",
      data: {
        text: this.data.text || ""
      },
      header: {
        'content-type': 'application/json'
      },
      success: (res) => {
        console.log('服务器回复', res)
      },
      fail: (err) => {
        console.log("连接服务器失败", err)
        wx.showToast({
          title: '无法连接服务器，请检查网络',
          icon: 'none',
          duration: 5000
        })
      }
    })
  },
  /**
   * 生命周期函数--监听页面加载
   */
  onLoad(options) {
    this.getSwiperList()
    manager.onStop = (res) => {
      this.setData({
        text: res.result
      })
      this.send_text()
      wx.hideLoading()
      setTimeout(() => {
        this.setData({
          text: "",
          showTextDisplay: false
        })
      }, 3000)
    }
    manager.onError=(err)=>{
      console.error("语音识别错误：", err)
      // 出错了也要强制关闭 loading！
      wx.hideLoading()
      wx.showToast({
        title: "录音失败：" + err.msg,
        icon: "none"
      })
      this.setData({
        showTextDisplay: false
      })
    }
  },

  /**
   * 生命周期函数--监听页面初次渲染完成
   */
  onReady() {

  },

  /**
   * 生命周期函数--监听页面隐藏
   */
  onHide() {

  },

  /**
   * 生命周期函数--监听页面卸载
   */
  onUnload() {

  },

  /**
   * 页面相关事件处理函数--监听用户下拉动作
   */
  onPullDownRefresh() {
    this.getSwiperList()
    console.log("用户下拉，页面重新加载")
  },

  /**
   * 页面上拉触底事件的处理函数
   */
  onReachBottom() {

  },

  /**
   * 用户点击右上角分享
   */
  onShareAppMessage() {

  },

  onShow() {
    // 启动轮询：每5秒请求一次
    this.startPolling()
  },
  onUnload() {
    // 页面销毁时必清定时器
    clearInterval(this.timer)
  },
  startPolling() {
    // 先清旧定时器，避免重复
    clearInterval(this.timer)
    this.timer = setInterval(() => {
      const app = getApp()
      app.get_alarm() // 调用全局函数
    }, 3000)
  }
})