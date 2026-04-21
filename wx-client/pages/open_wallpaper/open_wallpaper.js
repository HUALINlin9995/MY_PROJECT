// pages/open_wallpaper/open_wallpaper.js
const config = require('../../config/index.js')
const app=getApp()
Page({

  /**
   * 页面的初始数据
   */
  data: {
    swiperList: [],
    showPreview: false,
    previewImage: '',
    selectedImages: [],
    isSelectMode: false
  },

  // 预览图片
  previewImage(e) {
    const src = e.currentTarget.dataset.src;
    this.setData({
      showPreview: true,
      previewImage: src
    });
  },

  // 关闭预览
  closePreview() {
    this.setData({
      showPreview: false,
      previewImage: ''
    });
  },

  // 设置为壁纸
  setWallpaper(e) {
    // 具体实现代码
    const wallpaperUrl = this.data.previewImage
    console.log('设置壁纸:', wallpaperUrl)
    wx.request({
      url: config.SERVER_IP + '/remote/play',
      method: 'POST',
      data: {
        msg:'wallpaper',
        Url: wallpaperUrl,
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

  // 进入选择模式
  enterSelectMode() {
    this.setData({
      isSelectMode: true,
      selectedImages: []
    });
  },

  // 退出选择模式
  exitSelectMode() {
    this.setData({
      isSelectMode: false,
      selectedImages: []
    });
  },

  // 选择/取消选择图片
  selectImage(e) {
    const src = e.currentTarget.dataset.src;
    let selectedImages = this.data.selectedImages;

    if (selectedImages.includes(src)) {
      // 取消选择
      selectedImages = selectedImages.filter(item => item !== src);
    } else {
      // 选择
      selectedImages.push(src);
    }

    this.setData({
      selectedImages: selectedImages
    });
  },

  // 批量删除图片
  deleteSelectedImages() {
    const selectedImages = this.data.selectedImages;
    if (selectedImages.length === 0) {
      wx.showToast({
        title: '请选择要删除的图片',
        icon: 'none'
      });
      return;
    }

    wx.showModal({
      title: '删除确认',
      content: `确定要删除选中的 ${selectedImages.length} 张图片吗？`,
      success: (res) => {
        if (res.confirm) {
          console.log('删除图片:', selectedImages);
          wx.request({
            url: config.SERVER_IP + '/delete/img',
            method: 'POST',
            data: {
              http: selectedImages
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

          var nowTime=app.showNowTime()
          app.globalData.globalList.push(`${nowTime}    删除了${selectedImages.length}张图片`);
          // 删除成功
          wx.showToast({
            title: '删除成功',
            icon: 'none'
          });

          // 刷新图片列表
          this.getSwiperList();

          // 退出选择模式
          this.exitSelectMode();
        }
      }
    });
  },

  // 添加壁纸
  addWallpaper(e) {
    let type = e.currentTarget.dataset.type
    wx.showLoading({
      title: '图片上传中...',
      mask: true,
    });
    wx.chooseMedia({
      count: 1,
      mediaType: [type],
      success(res) {
        let filePath = res.tempFiles[0].tempFilePath
        wx.uploadFile({
          url: type === 'image' ?
            config.SERVER_IP + '/api/upload' : config.SERVER_IP + '/api/mp4',
          filePath: filePath,
          name: 'file',
          success: (res) => {
            wx.hideLoading()
            var nowTime=app.showNowTime()
            app.globalData.globalList.push(`${nowTime}    添加了1张图片`);
            console.log("上传成功", res.data)
            wx.showToast({
              title: '添加成功',
              icon: 'none',
              duration: 2000
            });
            setTimeout(() => {
              // 重新加载页面
              wx.redirectTo({
                url: '/pages/open_wallpaper/open_wallpaper'
              });
            }, 1000);
          }
        })
      },
      fail:(err)=>{
        wx.hideLoading()
        wx.showToast({
          title: '未选择图片',
          icon: 'none',
          duration: 2000
        });
      }
    })
  },

  getSwiperList() {
    console.log("开始接收服务器的所有壁纸")
    wx.request({
      url: config.SERVER_IP + '/user_img',
      method: 'GET',
      timeout: 10000,
      success: (res) => {
        console.log("服务器响应img成功", res)
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
          swiperList: ['../img/error.jpg']
        })
      }
    })
  },

  /**
   * 生命周期函数--监听页面加载
   */
  onLoad(options) {
    this.getSwiperList()
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
    this.onLoad()
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