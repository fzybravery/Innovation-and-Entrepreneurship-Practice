import cv2
import numpy as np
import matplotlib.pyplot as plt
import os

# --- 获取当前脚本所在目录 ---
# os.path.dirname(__file__) 获取当前文件的目录
# os.path.abspath() 获取绝对路径
SCRIPT_DIR = os.path.abspath(os.path.dirname(__file__))

# --- 配置参数 ---
# 宿主图像路径 (相对于脚本目录)
HOST_IMAGE_PATH = os.path.join(SCRIPT_DIR, 'host_image.png')
# 水印图像路径 (相对于脚本目录)
WATERMARK_IMAGE_PATH = os.path.join(SCRIPT_DIR, 'watermark.png')
# 输出文件夹路径 (在脚本目录下的output文件夹)
OUTPUT_DIR = os.path.join(SCRIPT_DIR, 'output')

# 如果输出文件夹不存在，则创建
if not os.path.exists(OUTPUT_DIR):
    os.makedirs(OUTPUT_DIR)

# 其他配置参数保持不变
# DCT块大小 (通常为8x8)
BLOCK_SIZE = 8
# 水印嵌入强度 (alpha值越大，水印越明显但鲁棒性越好)
ALPHA = 10
# 提取水印的阈值 (用于盲提取时判断是0还是1，需要根据alpha和实际效果调整)
EXTRACT_THRESHOLD = 0.5 * ALPHA

# --- 辅助函数 (保持不变) ---

def img_to_ycrcb(image):
    """将RGB图像转换为YCrCb颜色空间"""
    return cv2.cvtColor(image, cv2.COLOR_BGR2YCrCb)

def ycrcb_to_rgb(image_ycrcb):
    """将YCrCb图像转换回RGB颜色空间"""
    return cv2.cvtColor(image_ycrcb, cv2.COLOR_YCrCb2BGR)

def get_block_dct(channel, r, c):
    """获取指定块的DCT变换结果"""
    block = channel[r:r+BLOCK_SIZE, c:c+BLOCK_SIZE].astype(np.float32)
    dct_block = cv2.dct(block)
    return dct_block

def idct_block(dct_block):
    """对DCT块进行IDCT逆变换"""
    return cv2.idct(dct_block).astype(np.uint8)

def bin_to_image(binary_data, width, height):
    """将二进制数据重构为水印图像"""
    if not binary_data:
        return np.zeros((height, width), dtype=np.uint8)
    # 将二进制列表转换为NumPy数组，并重塑为图像尺寸
    image_array = np.array(binary_data).reshape((height, width)) * 255
    return image_array.astype(np.uint8)

def image_to_bin(image):
    """将灰度图像转换为二进制序列 (0或1)"""
    # 确保图像是灰度图且为二值 (0或255)
    _, binary_image = cv2.threshold(image, 128, 255, cv2.THRESH_BINARY)
    # 将255映射为1，0映射为0
    return (binary_image // 255).flatten().tolist()

def normalize_correlation(original_watermark, extracted_watermark):
    """
    计算两个二值图像的归一化相关系数 (Normalized Correlation, NC)。
    NC值越接近1，表示提取的水印与原始水印越相似。
    """
    if original_watermark.shape != extracted_watermark.shape:
        print("警告: 原始水印和提取水印尺寸不匹配，NC计算可能不准确。")
        # 尝试裁剪或填充以匹配最小尺寸
        h = min(original_watermark.shape[0], extracted_watermark.shape[0])
        w = min(original_watermark.shape[1], extracted_watermark.shape[1])
        original_watermark = original_watermark[:h, :w]
        extracted_watermark = extracted_watermark[:h, :w]

    orig_flat = original_watermark.flatten().astype(float)
    ext_flat = extracted_watermark.flatten().astype(float)

    # 归一化到0-1范围 (如果水印不是严格的0/255，这步很重要)
    orig_flat = orig_flat / 255.0
    ext_flat = ext_flat / 255.0

    mean_orig = np.mean(orig_flat)
    mean_ext = np.mean(ext_flat)

    numerator = np.sum((orig_flat - mean_orig) * (ext_flat - mean_ext))
    denominator = np.sqrt(np.sum((orig_flat - mean_orig)**2) * np.sum((ext_flat - mean_ext)**2))

    if denominator == 0:
        return 0.0 # 避免除以零
    return numerator / denominator

def bit_error_rate(original_watermark, extracted_watermark):
    """
    计算两个二值图像的比特错误率 (Bit Error Rate, BER)。
    BER值越接近0，表示错误越少。
    """
    if original_watermark.shape != extracted_watermark.shape:
        print("警告: 原始水印和提取水印尺寸不匹配，BER计算可能不准确。")
        h = min(original_watermark.shape[0], extracted_watermark.shape[0])
        w = min(original_watermark.shape[1], extracted_watermark.shape[1])
        original_watermark = original_watermark[:h, :w]
        extracted_watermark = extracted_watermark[:h, :w]

    # 将255的像素视为1，0的像素视为0
    orig_bits = (original_watermark // 255).flatten()
    ext_bits = (extracted_watermark // 255).flatten()

    total_bits = len(orig_bits)
    if total_bits == 0:
        return 0.0

    errors = np.sum(orig_bits != ext_bits)
    return errors / total_bits

# --- 水印嵌入函数 (保持不变) ---

def embed_watermark(host_image_path, watermark_image_path, alpha=ALPHA):
    """
    将水印图像嵌入到宿主图像中。
    水印嵌入到Y通道的DCT中频系数。
    """
    host_img = cv2.imread(host_image_path)
    watermark_img = cv2.imread(watermark_image_path, cv2.IMREAD_GRAYSCALE)

    if host_img is None:
        raise FileNotFoundError(f"无法加载宿主图像: {host_image_path}")
    if watermark_img is None:
        raise FileNotFoundError(f"无法加载水印图像: {watermark_image_path}")

    # 将水印图像转换为二值图像 (0或1)
    _, watermark_binary = cv2.threshold(watermark_img, 128, 255, cv2.THRESH_BINARY)
    watermark_bits = image_to_bin(watermark_binary)

    # 记录水印的原始尺寸，方便提取时重构
    wm_height, wm_width = watermark_img.shape[:2]

    # 将宿主图像转换为YCrCb颜色空间，处理亮度(Y)分量
    ycrcb_img = img_to_ycrcb(host_img)
    Y_channel = ycrcb_img[:, :, 0].astype(np.float32) # 使用float32进行DCT

    rows, cols = Y_channel.shape
    watermark_idx = 0
    total_watermark_bits = len(watermark_bits)

    # 遍历图像的每个BLOCK_SIZE x BLOCK_SIZE块
    for r in range(0, rows - BLOCK_SIZE + 1, BLOCK_SIZE):
        for c in range(0, cols - BLOCK_SIZE + 1, BLOCK_SIZE):
            if watermark_idx >= total_watermark_bits:
                break # 水印位已全部嵌入

            # 获取当前块的DCT系数
            block = Y_channel[r:r+BLOCK_SIZE, c:c+BLOCK_SIZE]
            dct_block = cv2.dct(block)

            # 选择一个中频系数进行嵌入 (例如，(2,2)位置)
            # 不同的嵌入位置会有不同的鲁棒性和不可见性权衡
            embed_pos_r, embed_pos_c = 2, 2 # 可以尝试其他中频位置如(3,3), (1,2)等

            # 嵌入水印位
            if watermark_bits[watermark_idx] == 1:
                dct_block[embed_pos_r, embed_pos_c] += alpha
            else:
                dct_block[embed_pos_r, embed_pos_c] -= alpha

            # 逆DCT变换回空间域
            modified_block = cv2.idct(dct_block)

            # 确保像素值在有效范围内 [0, 255]
            modified_block = np.clip(modified_block, 0, 255)

            # 更新Y通道
            Y_channel[r:r+BLOCK_SIZE, c:c+BLOCK_SIZE] = modified_block

            watermark_idx += 1
        if watermark_idx >= total_watermark_bits:
            break

    # 将修改后的Y通道放回YCrCb图像
    ycrcb_img[:, :, 0] = Y_channel.astype(np.uint8)
    watermarked_img = ycrcb_to_rgb(ycrcb_img)

    print(f"水印嵌入完成。嵌入了 {watermark_idx} 比特。")
    return watermarked_img, wm_width, wm_height

# --- 水印提取函数 (盲提取，保持不变) ---

def extract_watermark_blind(watermarked_image, wm_width, wm_height, alpha=ALPHA):
    """
    从含水印图像中盲提取水印。
    不依赖原始宿主图像。
    通过判断DCT中频系数是否大于某个阈值来提取。
    """
    ycrcb_img = img_to_ycrcb(watermarked_image)
    Y_channel = ycrcb_img[:, :, 0].astype(np.float32)

    rows, cols = Y_channel.shape
    extracted_watermark_bits = []

    # 遍历图像的每个BLOCK_SIZE x BLOCK_SIZE块
    for r in range(0, rows - BLOCK_SIZE + 1, BLOCK_SIZE):
        for c in range(0, cols - BLOCK_SIZE + 1, BLOCK_SIZE):
            # 提取当前块的DCT系数
            block = Y_channel[r:r+BLOCK_SIZE, c:c+BLOCK_SIZE]
            dct_block = cv2.dct(block)

            # 提取嵌入位置的系数
            extract_pos_r, extract_pos_c = 2, 2

            # 基于阈值判断水印位 (假设0对应减alpha，1对应加alpha)
            # 这里的阈值需要根据嵌入强度alpha和噪声情况调整
            if dct_block[extract_pos_r, extract_pos_c] > EXTRACT_THRESHOLD:
                extracted_watermark_bits.append(1)
            else:
                extracted_watermark_bits.append(0)

            # 如果提取的比特数足够重构水印，则停止
            if len(extracted_watermark_bits) >= wm_width * wm_height:
                break
        if len(extracted_watermark_bits) >= wm_width * wm_height:
            break

    # 确保提取的比特数与原始水印尺寸匹配
    if len(extracted_watermark_bits) > wm_width * wm_height:
        extracted_watermark_bits = extracted_watermark_bits[:wm_width * wm_height]
    elif len(extracted_watermark_bits) < wm_width * wm_height:
        # 如果提取的比特不够，用0填充
        extracted_watermark_bits.extend([0] * (wm_width * wm_height - len(extracted_watermark_bits)))

    # 将提取的二进制比特重构为水印图像
    extracted_watermark_img = bin_to_image(extracted_watermark_bits, wm_width, wm_height)
    print(f"水印盲提取完成。提取了 {len(extracted_watermark_bits)} 比特。")
    return extracted_watermark_img


# --- 鲁棒性测试函数 (修改了文件保存路径) ---

def run_robustness_test(watermarked_img, original_watermark_img, wm_width, wm_height, test_name, attack_func):
    """
    运行单个鲁棒性测试并评估结果。
    """
    print(f"\n--- 运行鲁棒性测试: {test_name} ---")
    attacked_img = attack_func(watermarked_img.copy())
    extracted_wm = extract_watermark_blind(attacked_img, wm_width, wm_height, alpha=ALPHA)

    # 保存攻击后的图像和提取的水印到OUTPUT_DIR
    attacked_output_path = os.path.join(OUTPUT_DIR, f"{test_name.replace(' ', '_').replace('(', '').replace(')', '').replace('%', '')}_attacked.png")
    extracted_wm_output_path = os.path.join(OUTPUT_DIR, f"{test_name.replace(' ', '_').replace('(', '').replace(')', '').replace('%', '')}_extracted_watermark.png")
    cv2.imwrite(attacked_output_path, attacked_img)
    cv2.imwrite(extracted_wm_output_path, extracted_wm)
    print(f"已保存攻击后的图像: {attacked_output_path}")
    print(f"已保存提取的水印: {extracted_wm_output_path}")

    nc_value = normalize_correlation(original_watermark_img, extracted_wm)
    ber_value = bit_error_rate(original_watermark_img, extracted_wm)

    print(f"  归一化相关系数 (NC): {nc_value:.4f}")
    print(f"  比特错误率 (BER): {ber_value:.4f}")
    return nc_value, ber_value, attacked_img, extracted_wm

# --- 各种攻击函数 (保持不变) ---

def attack_flip_horizontal(image):
    """水平翻转"""
    return cv2.flip(image, 1)

def attack_flip_vertical(image):
    """垂直翻转"""
    return cv2.flip(image, 0)

def attack_translate(image, dx=20, dy=20):
    """平移"""
    rows, cols = image.shape[:2]
    M = np.float32([[1, 0, dx], [0, 1, dy]])
    translated_img = cv2.warpAffine(image, M, (cols, rows), borderValue=(0,0,0)) # 填充黑色
    return translated_img

def attack_crop(image, crop_ratio=0.1):
    """裁剪 (从中心裁剪)"""
    rows, cols = image.shape[:2]
    crop_h = int(rows * crop_ratio)
    crop_w = int(cols * crop_ratio)
    # 裁剪中心区域
    cropped_img = image[crop_h:rows-crop_h, crop_w:cols-crop_w]
    # 将裁剪后的图像填充回原大小 (通常填充黑色或白色，或直接测试小图的提取)
    # 这里我们填充回原尺寸，因为水印提取是基于原始图像块的遍历
    padded_img = np.zeros_like(image)
    padded_img[crop_h:rows-crop_h, crop_w:cols-crop_w] = cropped_img
    return padded_img


def attack_contrast(image, alpha_contrast=1.5, beta_contrast=0):
    """调整对比度 (alpha_contrast > 1 增加对比度, < 1 降低对比度)"""
    # new_image = alpha_contrast * old_image + beta_contrast
    return cv2.convertScaleAbs(image, alpha=alpha_contrast, beta=beta_contrast)

def attack_brightness(image, beta_brightness=50):
    """调整亮度 (beta_brightness > 0 增加亮度, < 0 降低亮度)"""
    return cv2.convertScaleAbs(image, alpha=1.0, beta=beta_brightness)

def attack_gaussian_noise(image, mean=0, var=0.01):
    """添加高斯噪声"""
    sigma = var**0.5
    gauss = np.random.normal(mean, sigma, image.shape)
    noisy_img = image + gauss.reshape(image.shape)
    return np.clip(noisy_img, 0, 255).astype(np.uint8)

def attack_median_blur(image, ksize=3):
    """中值滤波"""
    return cv2.medianBlur(image, ksize)

def attack_gaussian_blur(image, ksize=(5, 5)):
    """高斯模糊"""
    return cv2.GaussianBlur(image, ksize, 0)

def attack_jpeg_compression(image, quality=50):
    """JPEG压缩"""
    # 将图像编码为JPEG格式到内存
    encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]
    _, encimg = cv2.imencode('.jpg', image, encode_param)
    # 解码回图像
    decompressed_img = cv2.imdecode(encimg, 1)
    return decompressed_img

# --- 主执行逻辑 (修改了文件保存路径) ---

def main():
    # 1. 加载原始宿主图像和水印图像
    original_host_img = cv2.imread(HOST_IMAGE_PATH)
    original_watermark_img = cv2.imread(WATERMARK_IMAGE_PATH, cv2.IMREAD_GRAYSCALE)

    # 如果图像不存在，则生成示例图像并保存到脚本目录下
    if original_host_img is None:
        print(f"错误: 无法加载宿主图像 {HOST_IMAGE_PATH}。正在生成示例图像...")
        original_host_img = np.zeros((256, 256, 3), dtype=np.uint8)
        cv2.putText(original_host_img, "Host Image", (50, 128), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
        cv2.imwrite(HOST_IMAGE_PATH, original_host_img)
        print(f"已生成示例宿主图像: {HOST_IMAGE_PATH}")

    if original_watermark_img is None:
        print(f"错误: 无法加载水印图像 {WATERMARK_IMAGE_PATH}。正在生成示例图像...")
        original_watermark_img = np.zeros((32, 32), dtype=np.uint8)
        cv2.putText(original_watermark_img, "WM", (5, 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (255, 255, 255), 1)
        _, original_watermark_img = cv2.threshold(original_watermark_img, 128, 255, cv2.THRESH_BINARY)
        cv2.imwrite(WATERMARK_IMAGE_PATH, original_watermark_img)
        print(f"已生成示例水印图像: {WATERMARK_IMAGE_PATH}")

    # 将原始水印图像转换为二值形式（0或255），用于后续比较
    _, original_watermark_binary = cv2.threshold(original_watermark_img, 128, 255, cv2.THRESH_BINARY)
    wm_height, wm_width = original_watermark_img.shape[:2]


    # 2. 嵌入水印
    print("\n--- 正在嵌入水印 ---")
    watermarked_image, _, _ = embed_watermark(HOST_IMAGE_PATH, WATERMARK_IMAGE_PATH, alpha=ALPHA)
    # 保存含水印图像到OUTPUT_DIR
    watermarked_image_output_path = os.path.join(OUTPUT_DIR, 'watermarked_image.png')
    cv2.imwrite(watermarked_image_output_path, watermarked_image)
    print(f"含水印图像已保存到: {watermarked_image_output_path}")

    # 3. 验证无攻击下的水印提取 (作为基准)
    print("\n--- 验证无攻击下的水印提取 ---")
    extracted_wm_no_attack = extract_watermark_blind(watermarked_image, wm_width, wm_height, alpha=ALPHA)
    # 保存无攻击提取的水印到OUTPUT_DIR
    cv2.imwrite(os.path.join(OUTPUT_DIR, "extracted_wm_no_attack.png"), extracted_wm_no_attack)
    nc_no_attack = normalize_correlation(original_watermark_binary, extracted_wm_no_attack)
    ber_no_attack = bit_error_rate(original_watermark_binary, extracted_wm_no_attack)
    print(f"  无攻击下 NC: {nc_no_attack:.4f}")
    print(f"  无攻击下 BER: {ber_no_attack:.4f}")


    # 4. 运行鲁棒性测试
    test_results = {}
    tests = {
        "Flip Horizontal": attack_flip_horizontal,
        "Flip Vertical": attack_flip_vertical,
        "Translate": attack_translate,
        "Crop (10% border)": lambda img: attack_crop(img, crop_ratio=0.1),
        "Contrast (High)": lambda img: attack_contrast(img, alpha_contrast=1.5),
        "Contrast (Low)": lambda img: attack_contrast(img, alpha_contrast=0.7),
        "Brightness (High)": lambda img: attack_brightness(img, beta_brightness=50),
        "Brightness (Low)": lambda img: attack_brightness(img, beta_brightness=-50),
        "Gaussian Noise": lambda img: attack_gaussian_noise(img, var=0.005), # 调整噪声强度
        "Median Blur (3x3)": lambda img: attack_median_blur(img, ksize=3),
        "Gaussian Blur (5x5)": lambda img: attack_gaussian_blur(img, ksize=(5, 5)),
        "JPEG Compression (Q=70)": lambda img: attack_jpeg_compression(img, quality=70),
        "JPEG Compression (Q=30)": lambda img: attack_jpeg_compression(img, quality=30),
    }

    for name, attack_func in tests.items():
        nc, ber, attacked_img, extracted_wm = run_robustness_test(watermarked_image, original_watermark_binary, wm_width, wm_height, name.replace(" ", "_").replace("(", "").replace(")", "").replace("%", ""), attack_func)
        test_results[name] = {"NC": nc, "BER": ber, "Attacked_Image": attacked_img, "Extracted_Watermark": extracted_wm}

    # 5. 可视化结果 (修改了图片保存路径)
    print("\n--- 可视化结果 ---")
    plt.figure(figsize=(15, 10))

    # 原始图像和水印
    plt.subplot(3, 4, 1)
    plt.imshow(cv2.cvtColor(original_host_img, cv2.COLOR_BGR2RGB))
    plt.title("Original Host Image")
    plt.axis('off')

    plt.subplot(3, 4, 2)
    plt.imshow(original_watermark_img, cmap='gray')
    plt.title("Original Watermark")
    plt.axis('off')

    # 含水印图像
    plt.subplot(3, 4, 3)
    plt.imshow(cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2RGB))
    plt.title("Watermarked Image")
    plt.axis('off')

    # 无攻击提取的水印
    plt.subplot(3, 4, 4)
    plt.imshow(extracted_wm_no_attack, cmap='gray')
    plt.title(f"Extracted WM (No Attack)\nNC:{nc_no_attack:.2f} BER:{ber_no_attack:.2f}")
    plt.axis('off')

    # 显示部分攻击结果
    display_attacks = [
        "JPEG Compression (Q=30)",
        "Gaussian Noise",
        "Contrast (Low)",
        "Crop (10% border)"
    ]

    plot_idx = 5
    for attack_name in display_attacks:
        if attack_name in test_results:
            result = test_results[attack_name]
            # 显示攻击后的图像
            plt.subplot(3, 4, plot_idx)
            plt.imshow(cv2.cvtColor(result["Attacked_Image"], cv2.COLOR_BGR2RGB))
            plt.title(f"{attack_name} (Attacked)")
            plt.axis('off')
            plot_idx += 1

            # 显示提取的水印
            plt.subplot(3, 4, plot_idx)
            plt.imshow(result["Extracted_Watermark"], cmap='gray')
            plt.title(f"{attack_name} (Extracted WM)\nNC:{result['NC']:.2f} BER:{result['BER']:.2f}")
            plt.axis('off')
            plot_idx += 1

    plt.tight_layout()
    # 保存汇总图到OUTPUT_DIR
    plt.savefig(os.path.join(OUTPUT_DIR, "robustness_summary.png"))
    plt.show()

    print("\n--- 鲁棒性测试结果汇总 ---")
    print(f"{'攻击类型':<25} {'NC值':<10} {'BER值':<10}")
    print("-" * 45)
    print(f"{'无攻击':<25} {nc_no_attack:<10.4f} {ber_no_attack:<10.4f}")
    for name, result in test_results.items():
        print(f"{name:<25} {result['NC']:<10.4f} {result['BER']:<10.4f}")

if __name__ == "__main__":
    main()