// pages/message/message.js
const config = require('../../config/index.js')
const app = getApp()
Page({

  /**
   * 页面的初始数据
   */
  data: {
    messageList: []
  },

  getMessage() {
    wx.request({
      url: config.SERVER_IP + '/api/message',
      method: "GET",
      timeout: 10000,
      success: (res) => {
        if (res.statusCode === 200) {
          console.log('收到数据', res)
          const newMsg = res.data.msg
          console.log('数据为', newMsg)
          if (newMsg && newMsg !== "") {
            if (Array.isArray(newMsg))
              app.globalData.globalList.push(...newMsg);
            else
              app.globalData.globalList.push(newMsg);
          }
          this.setData({
            messageList: app.globalData.globalList
          });
          wx.stopPullDownRefresh();
        }
      },
      fail: (err) => {
        console.error("拉取消息失败：", err);
        wx.showToast({
          title: '加载失败',
          icon: 'none'
        });
        wx.stopPullDownRefresh();
      }
    })
  },

  /**
   * 生命周期函数--监听页面加载
   */
  onLoad(options) {
    this.getMessage()
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
    this.getMessage()
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
    this.getMessage()
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