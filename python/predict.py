#!/usr/bin/env python3
import sys
import json
import os
from pathlib import Path
import numpy as np
import tensorflow as tf
from tensorflow import keras
from PIL import Image
import json as json_lib

def load_and_preprocess_image(image_path, target_size=(224, 224)):
    """
    画像を読み込んで前処理
    """
    img = Image.open(image_path)
    
    # RGBに変換（グレースケールやRGBAの場合）
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # リサイズ
    img = img.resize(target_size)
    
    # NumPy配列に変換して正規化
    img_array = np.array(img) / 255.0
    
    # バッチ次元を追加
    img_array = np.expand_dims(img_array, axis=0)
    
    return img_array

def predict_image(model_dir, image_path):
    """
    画像を分類
    """
    
    model_dir = Path(model_dir)
    
    # モデル読み込み
    print("モデル読み込み中...")
    model_path = model_dir / 'best_model.h5'
    
    if not model_path.exists():
        model_path = model_dir / 'final_model.h5'
    
    if not model_path.exists():
        raise FileNotFoundError("モデルファイルが見つかりません")
    
    model = keras.models.load_model(str(model_path))
    
    # クラス名読み込み
    with open(model_dir / 'class_names.json', 'r', encoding='utf-8') as f:
        class_names = json_lib.load(f)
    
    # 画像の前処理
    print("画像処理中...")
    img_array = load_and_preprocess_image(image_path)
    
    # 推論
    print("推論中...")
    predictions = model.predict(img_array, verbose=0)
    
    # 結果の整形
    predicted_class_idx = np.argmax(predictions[0])
    confidence = predictions[0][predicted_class_idx]
    predicted_class_name = class_names[str(predicted_class_idx)]
    
    print(f"\n予測結果: {predicted_class_name}")
    print(f"信頼度: {confidence*100:.2f}%")
    print("\n全クラスのスコア:")
    
    # 全クラスのスコアを表示
    results = []
    for idx, score in enumerate(predictions[0]):
        class_name = class_names[str(idx)]
        print(f"  {class_name}: {score*100:.2f}%")
        results.append({
            'class': class_name,
            'confidence': float(score)
        })
    
    # 結果をJSON形式で出力
    output = {
        'predicted_class': predicted_class_name,
        'confidence': float(confidence),
        'all_predictions': results
    }
    
    print("\n" + json_lib.dumps(output, ensure_ascii=False, indent=2))
    
    return output

def main():
    if len(sys.argv) < 2:
        print("引数が必要です")
        sys.exit(1)
    
    try:
        args = json.loads(sys.argv[1])
        model_dir = args.get('model_dir')
        image_path = args.get('image_path')
        
        # GPU設定
        gpus = tf.config.list_physical_devices('GPU')
        if gpus:
            for gpu in gpus:
                tf.config.experimental.set_memory_growth(gpu, True)
        
        result = predict_image(model_dir, image_path)
        
        # Qt側で読み取りやすい形式で最終結果を出力
        print(f"RESULT: {result['predicted_class']} ({result['confidence']*100:.1f}%)")
        
    except Exception as e:
        print(f"エラー: {str(e)}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    main()