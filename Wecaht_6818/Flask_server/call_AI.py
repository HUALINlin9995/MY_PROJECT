from openai import OpenAI
from openai import APIStatusError, AuthenticationError

QWEN_API_KEY = "API_KEY"

client = OpenAI(
    api_key=QWEN_API_KEY,
    base_url="https://dashscope.aliyuncs.com/compatible-mode/v1",
    timeout=30
)

#专属知识库
DEV_BOARD_SYSTEM_PROMPT = """
你是粤嵌6818 Linux开发板的专属命令助手，严格遵守以下规则：
1. 只输出可直接在开发板终端执行的Linux命令，无解释、无多余文字
2. 只能使用我提供的路径，不编造
3. 不知道命令回复：未知命令
粤嵌6818路径：/root/my_project
视频存放路径：/ceshi/movie
音频存放路径：/ceshi/audio
视频路径下存在(20260417_231104.mp4)视频
音频有：(20260417_222359.mp3)
"""

#调用大模型
def get_dev_board_command(user_question: str) -> str:
    try:
        print('开始调用api')
        response = client.chat.completions.create(
            model="qwen-plus",
            messages=[
                {"role": "system", "content": DEV_BOARD_SYSTEM_PROMPT},
                {"role": "user", "content": user_question}
            ],
            temperature=0.05,
            max_tokens=100
        )
        return response.choices[0].message.content.strip()
    except Exception as e:
        return f"错误：{str(e)}"
