// pages/open_video/open_video.js
const config = require('../../config/index.js')
const app = getApp()
Page({

  /**
   * 页面的初始数据
   */
  data: {
    swiperList: [],
    showTimePicker: false,
    pickerValue: [0],
    timeOptions: ['播放一次', '1分钟', '2分钟', '3分钟', '4分钟', '5分钟', '6分钟', '7分钟', '8分钟', '9分钟', '10分钟'],
    selectedVideoIndex: -1
  },

  //远程播放按钮
  onRemotePlay(e) {
    const index = e.currentTarget.dataset.index
    const videoUrl = this.data.swiperList[index]
    console.log('远程播放视频:', videoUrl)

    // 保存当前选择的视频索引
    this.setData({
      selectedVideoIndex: index,
      showTimePicker: true,
      pickerValue: [0]
    })
  },

  // 关闭时间选择器
  closeTimePicker() {
    this.setData({
      showTimePicker: false,
      selectedVideoIndex: -1
    })
  },

  // 时间选择器值变化
  onTimeChange(e) {
    this.setData({
      pickerValue: e.detail.value
    })
  },

  // 确认时间选择
  confirmTimeSelection() {
    const selectedIndex = this.data.pickerValue[0]
    const videoIndex = this.data.selectedVideoIndex
    const videoUrl = this.data.swiperList[videoIndex]

    let playTime = 0
    let playMode = ''

    if (selectedIndex === 0) {
      playMode = 'once'
      playTime = 0
    } else {
      playMode = 'hours'
      playTime = selectedIndex
    }

    console.log('选择的播放时间:', playMode === 'once' ? '播放一次' : `${playTime}分钟`)

    // 记录日志
    var nowTime = app.showNowTime()
    app.globalData.globalList.push(`${nowTime}    远程播放视频: ${playMode === 'once' ? '播放一次' : `${playTime}分钟`}`);

    // 向服务器发送远程播放请求
    wx.request({
      url: config.SERVER_IP + '/remote/play',
      method: 'POST',
      data: {
        msg: 'movie',
        Url: videoUrl,
        playMode: playMode,
        playTime: playTime
      },
      header: {
        'content-type': 'application/json'
      },
      success: (res) => {
        console.log('远程播放响应:', res.data)
        wx.showToast({
          title: `设置成功: ${playMode === 'once' ? '播放一次' : `${playTime}分钟`}`,
          icon: 'success'
        })
      },
      fail: (err) => {
        console.log('远程播放失败:', err)
        wx.showToast({
          title: '远程播放设置失败',
          icon: 'none'
        })
      }
    })

    // 关闭时间选择器
    this.closeTimePicker()
  },

  //删除视频按钮
  onDeleteVideo(e) {
    const index = e.currentTarget.dataset.index
    const videoUrl = this.data.swiperList[index]

    wx.showModal({
      title: '删除确认',
      content: '确定要删除这个视频吗？',
      confirmText: '删除',
      cancelText: '取消',
      success: (res) => {
        // 用户点击了【确定】才执行删除
        if (res.confirm) {
          console.log('删除视频:', videoUrl)

          // 停止视频播放并释放资源
          const videoCtx = wx.createVideoContext(`video-${index}`, this);
          if (videoCtx) {
            videoCtx.pause();
            videoCtx.stop();
          }

          // 从本地列表中删除视频
          const newSwiperList = this.data.swiperList.filter((_, i) => i !== index)
          this.setData({
            swiperList: newSwiperList
          })

          // 记录日志
          var nowTime = app.showNowTime()
          app.globalData.globalList.push(`${nowTime}    删除了1个视频`);

          wx.showToast({
            title: '删除成功',
            icon: 'success',
            duration: 2000
          })

          // 向服务器发送删除请求
          wx.request({
            url: config.SERVER_IP + '/delete/movie',
            method: 'POST',
            data: {
              http: videoUrl
            },
            header: {
              'content-type': 'application/json'
            },
            success: (res) => {
              console.log('服务器删除响应:', res.data)
            },
            fail: (err) => {
              console.log('服务器删除失败:', err)
            }
          })
        }
        // 用户点击取消 → 什么都不做
        else if (res.cancel) {
          console.log('用户取消删除')
        }
      },
    })
  },

  //接收视频
  getswiperList() {
    console.log('开始接收视频')
    wx.request({
      url: config.SERVER_IP + '/user_movie',
      method: 'GET',
      timeout: 10000,
      success: (res) => {
        console.log('服务器响应成功', res)
        if (res.statusCode === 200) {
          this.setData({
            swiperList: res.data.data
          })
          wx.stopPullDownRefresh()
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
          swiperList: ['../img/network_error.jpg']
        })
      }
    })
  },

  //添加视频
  addMovie(e) {
    let type = e.currentTarget.dataset.type
    wx.showLoading({
      title: '视频上传中...',
      mask: true,
    });
    wx.chooseMedia({
      count: 1,
      mediaType: ['video'],
      success(res) {
        let filePath = res.tempFiles[0].tempFilePath
        wx.uploadFile({
          url: config.SERVER_IP + '/api/mp4',
          filePath: filePath,
          name: 'file',
          success: (res) => {
            wx.hideLoading();
            var nowTime = app.showNowTime()
            app.globalData.globalList.push(`${nowTime}    添加了1个视频`);
            console.log("上传成功", res.data)
            wx.showToast({
              title: '添加成功',
              icon: 'none',
              duration: 2000
            });
            setTimeout(() => {
              // 重新加载页面
              wx.redirectTo({
                url: '/pages/open_video/open_video'
              });
            }, 1000);
          },
          fail: (err) => {
            wx.hideLoading();
            wx.showToast({
              title: '上传失败',
              icon: 'none',
              duration: 2000
            });
          }
        })
      },
      fail: (err) => {
        wx.hideLoading();
        wx.showToast({
          title: '未选择视频',
          icon: 'none',
          duration: 2000
        });
      }
    })
  },

  /**
   * 生命周期函数--监听页面加载
   */
  onLoad(options) {
    this.getswiperList()
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
    this.getswiperList()
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