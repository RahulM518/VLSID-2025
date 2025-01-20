from flask import Flask, render_template_string, Response
import cv2
from ultralytics import YOLO
import time

# Initialize Flask application
app = Flask(__name__)

# Camera URL (your MJPEG stream)
CAMERA_URL = "http://192.168.123.197:8080/video"

# Load YOLO model (initialized only once when the app starts)
model = YOLO('best.pt')  # Load your YOLO weights file

# Initialize video capture
cap = cv2.VideoCapture(CAMERA_URL)

# HTML Template for displaying video feed
index_html = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Live Video Stream with YOLOv8 Detection</title>
</head>
<body>
    <h1>Live Video Stream with YOLOv8 Object Detection</h1>
    <img src="{{ url_for('video_feed') }}" width="640" height="480">
</body>
</html>
"""

def generate_frames(skip_frames=5, target_fps=1):
    """
    Captures, processes, and streams frames, skipping intermediate frames or limiting FPS.
    Args:
        skip_frames (int): Process every nth frame (default: 5).
        target_fps (int): Limit processing to this number of frames per second (default: 1).
    """
    frame_count = 0
    frame_interval = 1 / target_fps  # Calculate time interval between frames
    last_time = time.time()

    while True:
        ret, frame = cap.read()
        if not ret:
            print("Failed to read frame from camera stream.")
            break

        frame_count += 1
        current_time = time.time()

        # Skip frames if necessary
        if frame_count % skip_frames != 0:
            continue

        # Ensure frame rate limit
        if current_time - last_time < frame_interval:
            continue

        last_time = current_time  # Update time for frame rate limiting

        # Perform YOLO object detection
        results = model(frame)
        frame = results[0].plot()  # Get the frame with bounding boxes drawn

        # Encode the processed frame as JPEG
        ret, buffer = cv2.imencode('.jpg', frame)
        if not ret:
            print("Failed to encode frame.")
            continue

        frame_bytes = buffer.tobytes()

        # Yield the processed frame to Flask for streaming
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

        # Log the processing information
        print(f"Frame processed and streamed. Skipping: {skip_frames}, Target FPS: {target_fps}")

@app.route('/')
def index():
    """
    Renders the main page with the video feed.
    """
    return render_template_string(index_html)

@app.route('/video_feed')
def video_feed():
    """
    Returns the video feed with YOLO detections.
    """
    return Response(generate_frames(skip_frames=5, target_fps=5),  # Adjust values here
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    try:
        # Start the Flask app on port 5000
        app.run(debug=True, host="0.0.0.0", port=5000)
    finally:
        cap.release()  # Ensure the video capture is released when the server exits


