const app = getApp();
const config = require("../../config/index.js")
Component({
  properties: {
    text: { type: String, value: "系统报警提示" }
  },
  data: {
    show: true
  },

  // 生命周期：每次页面显示，自动同步全局状态（修复切换页面不刷新的BUG）
  lifetimes: {
    attached() {
      this.syncGlobalStatus();
    },
    show() {
      this.syncGlobalStatus();
    }
  },

  pageLifetimes: {
    // 页面显示时，强制同步全局开关
    show() {
      this.syncGlobalStatus();
    }
  },

  methods: {
    // 同步全局状态
    syncGlobalStatus() {
      this.setData({
        show: app.globalData.globalShowAlert
      });
    },

    // 点击关闭：修改全局变量，所有页面永久隐藏
    onClose() {
      app.globalData.globalShowAlert = false;
      this.syncGlobalStatus();
    },
  },

});