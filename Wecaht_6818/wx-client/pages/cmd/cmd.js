const config = require("../../config/index.js")
const app =getApp()
// pages/cmd/cmd.js
Page({

  /**
   * 页面的初始数据
   */
  data: {
    isLock: false,
    inputValue: '',
    commandHistory: [],
    timer: null
  },

  input_cmd: function (options) {
    if (this.data.isLock) return
    this.setData({
      isLock: true
    })
    let cmd = options.detail.value
    if (cmd) {
      console.log('输入的内容为', cmd)
      if (cmd === 'cls') {
        // 清屏命令
        this.setData({
          commandHistory: []
        })
        this.data.isLock = false;
        this.setData({
          inputValue: ''
        })
        return
      }
      // 将命令添加到历史记录
      let history = this.data.commandHistory
      history.push({ type: 'user', content: cmd })
      wx.request({
        url: config.SERVER_IP + '/api/input_cmd',
        method: 'POST',
        data: {
          cmd: cmd
        },
        header: {
          'content-type': 'application/json'
        },
        success: (res) => {
          console.log('服务器回复', res)
          this.setData({
            commandHistory: history
          })
        },
        fail: (err) => {
          console.log('请求失败', err)
          wx.showToast({
            title: '服务器异常',
            icon: 'none',
            duration: 2000
          })
        }
      })
    }

    this.setData({
      inputValue: ''
    })
    setTimeout(() => {
      this.setData({
        isLock: false
      });
    }, 300);
  },


  /**
   * 生命周期函数--监听页面加载
   */
  onLoad(options) {

  },

  /**
   * 生命周期函数--监听页面初次渲染完成
   */
  onReady() {

  },

  /**
   * 生命周期函数--监听页面显示
   */
  onShow() {
    // 启动定时请求
    this.startTimer()
  },

  /**
   * 启动定时请求
   */
  startTimer() {
    this.data.timer = setInterval(() => {
      this.checkServerOutput()
    }, 2000)
  },

  /**
   * 检查服务器输出
   */
  checkServerOutput() {
    console.log("发送一次cmd请求")
    wx.request({
      url: config.SERVER_IP+'/wx_msg',
      method: 'GET',
      header: {
        'content-type': 'application/json'
      },
      success: (res) => {
        if (res.data && res.data.msg) {
          // 有数据，显示在显示框内
          let msg = res.data.msg.replace(/\n/g, '|') // 去除换行符
          let history = this.data.commandHistory
          history.push({ type: 'server', content: msg })
          this.setData({
            commandHistory: history
          })
        }
      },
      fail: (err) => {
        console.log('请求服务器输出失败', err)
      }
    })
  },

  /**
   * 生命周期函数--监听页面隐藏
   */
  onHide() {
    // 清除定时器
    this.clearTimer()
  },

  /**
   * 生命周期函数--监听页面卸载
   */
  onUnload() {
    // 清除定时器
    this.clearTimer()
  },

  /**
   * 清除定时器
   */
  clearTimer() {
    if (this.data.timer) {
      clearInterval(this.data.timer)
      this.data.timer = null
    }
  },

  /**
   * 页面相关事件处理函数--监听用户下拉动作
   */
  onPullDownRefresh() {

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

  }
})