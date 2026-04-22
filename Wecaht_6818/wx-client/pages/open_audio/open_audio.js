const config = require('../../config/index.js')
const app=getApp()

Page({
  data: {
    audioList: [],
    playingIndex: -1,
    progress: 0,
    currentTime: '00:00',
    duration: '00:00',
    audioContext: null,
    // 录音相关状态
    isRecording: false,
    recordTime: 0,
    recordFilePath: '',
    showRecordPanel: false,
    recordTimer: null,
    previewing: false,
    previewContext: null
  },

  // 获取音频列表
  getAudioList() {
    wx.request({
      url: config.SERVER_IP + '/user_audio',
      method: 'GET',
      success: (res) => {
        console.log('获取音频列表响应:', res)
        if (res.statusCode === 200 && res.data.code === 200) {
          // 处理音频数据，提取原文件名
          const processedList = (res.data.data || []).map(url => {
            // 从URL中提取文件名
            const filename = url.split('/').pop()
            // 移除时间戳前缀，获取原文件名
            const originalName = this.extractOriginalName(filename)
            return {
              url: url,
              originalName: originalName,
              name: originalName 
            }
          })

          this.setData({
            audioList: processedList
          })
          console.log('音频列表更新成功:', this.data.audioList)
        } else {
          console.error('获取音频列表失败:', res.data)
          wx.showToast({
            title: res.data.msg || '获取列表失败',
            icon: 'error'
          })
        }
      },
      fail: (err) => {
        console.error('获取音频列表网络错误:', err)
        wx.showToast({
          title: '网络错误',
          icon: 'error'
        })
      }
    })
  },

  // 从文件名中提取原文件名
  extractOriginalName(filename) {
    if (!filename) return '未命名音频'
    // 移除文件扩展名
    const nameWithoutExt = filename.replace(/\.[^/.]+$/, "")
    const timestampMatch = nameWithoutExt.match(/^(\d{8}_\d{6})/)
    if (timestampMatch) {
      const timestamp = timestampMatch[1]
      const date = timestamp.substring(0, 8)
      const time = timestamp.substring(9)
      return `音频_${date.substring(0,4)}-${date.substring(4,6)}-${date.substring(6,8)}`
    }
    return nameWithoutExt || '未命名音频'
  },

  // 上传音频
  uploadAudio() {
    console.log('开始上传音频...')
    wx.chooseMedia({
      count: 1,
      mediaType: ['audio'],
      success: (res) => {
        const tempFile = res.tempFiles[0]
        const filePath = tempFile.tempFilePath

        console.log('选择的文件信息:', tempFile)

        const uploadTask = wx.uploadFile({
          url: config.SERVER_IP + '/api/audio',
          filePath: filePath,
          name: 'file',
          success: (res) => {
            this.getAudioList()
            var nowTime=app.showNowTime()
            app.globalData.globalList.push(`${nowTime}    添加了1个音频`);
            console.log("上传成功", res.data)
            wx.showToast({
              title: '添加成功',
              icon: 'none',
              duration: 2000
            });
          },
          fail: (err) => {
            wx.showToast({
              title: '添加失败',
              icon: 'none',
              duration: 2000
            });
          }
        })
      },
      fail: (err) => {
        console.error('选择文件失败:', err)
        wx.showToast({
          title: '选择文件失败',
          icon: 'error'
        })
      }
    })
  },

  // 显示录音面板
  showRecordPanel() {
    this.setData({
      showRecordPanel: true,
      isRecording: false,
      recordTime: 0,
      recordFilePath: '',
      previewing: false
    })
  },

  // 隐藏录音面板
  hideRecordPanel() {
    // 停止录音
    if (this.data.isRecording) {
      this.stopRecord()
    }
    // 停止预览
    if (this.data.previewing && this.data.previewContext) {
      this.data.previewContext.stop()
      this.data.previewContext.destroy()
      this.setData({ previewing: false, previewContext: null })
    }
    this.setData({ showRecordPanel: false })
  },

  // 开始录音
  startRecord() {
    console.log('开始录音...')
    wx.getRecorderManager().start({
      duration: 60000, // 最长60秒
      sampleRate: 44100,
      numberOfChannels: 1,
      encodeBitRate: 192000,
      format: 'aac',
      frameSize: 50
    })

    this.setData({ isRecording: true, recordTime: 0 })

    // 开始计时
    const recordTimer = setInterval(() => {
      this.setData({
        recordTime: this.data.recordTime + 1
      })
      // 达到60秒自动停止
      if (this.data.recordTime >= 60) {
        this.stopRecord()
      }
    }, 1000)

    this.setData({ recordTimer })

    // 监听录音结束事件
    wx.getRecorderManager().onStop((res) => {
      console.log('录音结束:', res)
      this.setData({ recordFilePath: res.tempFilePath })
    })
  },

  // 停止录音
  stopRecord() {
    console.log('停止录音...')
    wx.getRecorderManager().stop()
    
    // 清除计时器
    if (this.data.recordTimer) {
      clearInterval(this.data.recordTimer)
      this.setData({ recordTimer: null })
    }
    
    this.setData({ isRecording: false })
  },

  // 预览录音
  previewRecord() {
    if (this.data.previewing) {
      // 停止预览
      if (this.data.previewContext) {
        this.data.previewContext.stop()
        this.data.previewContext.destroy()
      }
      this.setData({ previewing: false, previewContext: null })
    } else {
      // 开始预览
      const previewContext = wx.createInnerAudioContext()
      previewContext.src = this.data.recordFilePath
      previewContext.play()
      
      previewContext.onEnded(() => {
        this.setData({ previewing: false, previewContext: null })
      })
      
      this.setData({ previewing: true, previewContext })
    }
  },

  // 确认发送录音
  sendRecord() {
    console.log('发送录音...')
    const tempFilePath = this.data.recordFilePath
    
    // 上传录音文件
    const uploadTask = wx.uploadFile({
      url: config.SERVER_IP + '/api/audio',
      filePath: tempFilePath,
      name: 'file',
      success: (res) => {
        this.getAudioList()
        var nowTime=app.showNowTime()
        app.globalData.globalList.push(`${nowTime}    添加了1个音频`);
        console.log("录音上传成功", res.data)
        wx.showToast({
          title: '录音添加成功',
          icon: 'none',
          duration: 2000
        });
        // 关闭录音面板
        this.hideRecordPanel()
      },
      fail: (err) => {
        wx.showToast({
          title: '录音添加失败',
          icon: 'none',
          duration: 2000
        });
      }
    })
  },

  // 远程播放音频
  playRemoteAudio(e) {
    const index = e.currentTarget.dataset.index
    const audioUrl = this.data.audioList[index].url
    console.log('远程播放音频 URL:', audioUrl)
    wx.request({
      url: config.SERVER_IP + '/remote/play',
      method: 'POST',
      data: {
        msg:'audio',
        Url: audioUrl,
      },
      header: {
        'content-type': 'application/json'
      },
      success:(res)=>{
        console.log("服务器回复",res)
        wx.showToast({
          'title':"设置成功",
          icon:'success',
          duration:3000
        })
      },
      fail:(err)=>{
        console.log("发送失败")
        wx.showToast({
          'title':'网络错误',
          icon:'none',
          duration:5000
        })
      }
    })
  },

  // 播放/暂停切换
  togglePlay(e) {
    const index = e.currentTarget.dataset.index
    const audioUrl = this.data.audioList[index].url

    console.log('切换播放状态:', {
      index,
      audioUrl
    })

    if (this.data.audioContext) {
      this.data.audioContext.stop()
      this.data.audioContext.destroy()
    }

    if (this.data.playingIndex === index) {
      this.setData({
        playingIndex: -1,
        progress: 0,
        currentTime: '00:00',
        duration: '00:00',
        audioContext: null
      })
      console.log('暂停播放')
    } else {
      const audioContext = wx.createInnerAudioContext()

      audioContext.onCanplay(() => {
        console.log('音频可以播放')
        audioContext.play()
      })

      audioContext.onTimeUpdate(() => {
        const current = audioContext.currentTime
        const duration = audioContext.duration
        const progress = duration > 0 ? (current / duration) * 100 : 0

        this.setData({
          progress: Math.min(progress, 100),
          currentTime: this.formatTime(current),
          duration: this.formatTime(duration)
        })
      })

      audioContext.onEnded(() => {
        console.log('播放结束')
        this.setData({
          playingIndex: -1,
          progress: 0,
          currentTime: '00:00',
          duration: '00:00',
          audioContext: null
        })
      })

      audioContext.onError((res) => {
        console.error('播放错误:', res)
        wx.showToast({
          title: '播放失败',
          icon: 'error'
        })
        this.setData({
          playingIndex: -1,
          audioContext: null
        })
      })

      audioContext.src = audioUrl

      this.setData({
        playingIndex: index,
        audioContext: audioContext
      })

      console.log('设置播放状态:', index)
    }
  },

  // 删除音频
  deleteAudio(e) {
    const index = e.currentTarget.dataset.index
    const audioUrl = this.data.audioList[index].url
    wx.showModal({
      title: '确认删除',
      content: '确定要删除这个音频吗？',
      success: (res) => {
        var nowTime=app.showNowTime()
        app.globalData.globalList.push(`${nowTime}    删除了1个音频`);
        if (res.confirm) {
          if (this.data.playingIndex === index && this.data.audioContext) {
            this.data.audioContext.stop()
            this.data.audioContext.destroy()
          }
          const newList = this.data.audioList.filter((_, i) => i !== index)
          this.setData({
            audioList: newList,
            playingIndex: this.data.playingIndex === index ? -1 : this.data.playingIndex
          })
          wx.request({
            url: config.SERVER_IP + '/delete/audio',
            method: 'POST',
            data: {
              http: audioUrl
            },
            header: {
              'content-type': 'application/json'
            },
            success: (res) => {
              console.log('删除响应:', res)
              if (res.data.code === 200) {
                wx.showToast({
                  title: '删除成功',
                  icon: 'success'
                })
              } else {
                wx.showToast({
                  title: '删除失败',
                  icon: 'error'
                })
              }
            },
            fail: (err) => {
              console.error('删除请求失败:', err)
              wx.showToast({
                title: '删除失败',
                icon: 'error'
              })
            }
          })
        }
      }
    })
  },

  // 格式化时间
  formatTime(seconds) {
    if (!seconds || seconds === 0 || isNaN(seconds)) return '00:00'
    const mins = Math.floor(seconds / 60)
    const secs = Math.floor(seconds % 60)
    return `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`
  },

  onLoad(options) {
    console.log('页面加载，开始获取音频列表')
    this.getAudioList()
  },

  onShow() {
    console.log('页面显示，刷新音频列表')
    this.getAudioList()
  },

  onPullDownRefresh() {
    console.log('下拉刷新')
    this.getAudioList()
    wx.stopPullDownRefresh()
  },

  onUnload() {
    console.log('页面卸载，清理资源')
    if (this.data.audioContext) {
      this.data.audioContext.stop()
      this.data.audioContext.destroy()
    }
  }
})