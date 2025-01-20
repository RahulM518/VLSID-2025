
from flask import Flask, render_template_string, Response, request, jsonify
import cv2
from ultralytics import YOLO
import time
import socket

# Initialize Flask application
app = Flask(__name__)

# Camera URL (your MJPEG stream)
CAMERA_URL = "http://192.168.123.197:8080/video"

# ESP32 IPs
ESP32_1_IP = "192.168.123.83"
ESP32_2_IP = "192.168.123.30"
ESP32_PORT = 8080

# Load YOLO model
model = YOLO('best.pt')  # Load your YOLO weights file

# HTML Template for displaying two video feeds and ESP32 control panel
index_html = """
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Live Video Streams</title>
    <script>
        function sendCommand(formId, esp32Id) {
            const form = document.getElementById(formId);
            const command = form.elements["command"].value;

            fetch(`/send_command/${esp32Id}`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ command: command })
            })
            .then(response => response.json())
            .then(data => {
                const resultDiv = document.getElementById(`response_${esp32Id}`);
                resultDiv.innerHTML = `<strong>${data.message}</strong>`;
            })
            .catch(error => console.error('Error:', error));

            return false;  // Prevent form submission
        }
    </script>
</head>
<body>
    <h1>Live Video Streams</h1>
    <div>
        <h2>Object Detection</h2>
        <img src="{{ url_for('video_feed_detection') }}" width="640" height="480">
    </div>
    <div>
        <h2>Object Tracking</h2>
        <img src="{{ url_for('video_feed_tracking') }}" width="640" height="480">
    </div>
    <hr>
    <h1>ESP32 Control Panel</h1>

    <!-- ESP32 1 Form -->
    <form id="form_1" onsubmit="return sendCommand('form_1', 1);">
        <label for="command">Command for ESP32 (192.168.123.83):</label>
        <input type="text" id="command" name="command" placeholder="Enter command for ESP32 1">
        <button type="submit">Send</button>
    </form>
    <div id="response_1"></div>

    <hr>

    <!-- ESP32 2 Form -->
    <form id="form_2" onsubmit="return sendCommand('form_2', 2);">
        <label for="command">Command for ESP32 (192.168.123.30):</label>
        <input type="text" id="command" name="command" placeholder="Enter command for ESP32 2">
        <button type="submit">Send</button>
    </form>
    <div id="response_2"></div>
</body>
</html>
"""

def send_command_to_esp32(ip, command):
    """Send a command to the specified ESP32 device using a raw TCP socket."""
    try:
        # Create a socket connection
        client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client_socket.connect((ip, ESP32_PORT))

        # Send the command
        client_socket.sendall(f"{command}\n".encode())

        # Receive the response
        response = client_socket.recv(1024).decode()
        client_socket.close()
        return f"Response from {ip}: {response}"
    except socket.error as e:
        return f"Socket error while sending command to {ip}: {e}"
    except Exception as e:
        return f"Unexpected error while sending command to {ip}: {e}"

@app.route('/', methods=['GET'])
def index():
    return render_template_string(index_html)

@app.route('/send_command/<int:esp32_id>', methods=['POST'])
def send_command(esp32_id):
    data = request.get_json()
    command = data.get('command', '').strip().upper()

    if esp32_id == 1:
        response = send_command_to_esp32(ESP32_1_IP, command)
    elif esp32_id == 2:
        response = send_command_to_esp32(ESP32_2_IP, command)
    else:
        response = "Invalid ESP32 ID."

    return jsonify({"message": response})

def generate_detection_frames(skip_frames=5, target_fps=0.05):
    cap_detection = cv2.VideoCapture(CAMERA_URL)
    frame_count = 0
    frame_interval = 1 / target_fps
    last_time = time.time()

    while True:
        ret, frame = cap_detection.read()
        if not ret:
            print("[Detection] Failed to read frame from camera stream.")
            break

        frame_count += 1
        current_time = time.time()

        if frame_count % skip_frames != 0 or current_time - last_time < frame_interval:
            continue

        last_time = current_time

        results = model(frame)
        frame = results[0].plot()

        ret, buffer = cv2.imencode('.jpg', frame)
        if not ret:
            continue

        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

    cap_detection.release()

def generate_tracking_frames(skip_frames=5, target_fps=5):
    cap_tracking = cv2.VideoCapture(CAMERA_URL)
    frame_count = 0
    frame_interval = 1 / target_fps
    last_time = time.time()
    tracked_points = []

    while True:
        ret, frame = cap_tracking.read()
        if not ret:
            print("[Tracking] Failed to read frame from camera stream.")
            break

        frame_count += 1
        current_time = time.time()

        if frame_count % skip_frames != 0 or current_time - last_time < frame_interval:
            continue

        last_time = current_time

        results = model(frame)
        detections = results[0].boxes.xywh

        for det in detections:
            x, y, w, h = det[:4]
            center_x, center_y = int(x), int(y)
            tracked_points.append((center_x, center_y))

            if len(tracked_points) > 10:
                tracked_points.pop(0)

        for i in range(len(tracked_points) - 1):
            cv2.line(frame, tracked_points[i], tracked_points[i + 1], (0, 255, 0), 2)
        for point in tracked_points:
            cv2.circle(frame, point, 5, (0, 255, 0), -1)

        ret, buffer = cv2.imencode('.jpg', frame)
        if not ret:
            continue

        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')

    cap_tracking.release()

@app.route('/video_feed_detection')
def video_feed_detection():
    return Response(generate_detection_frames(skip_frames=5, target_fps=5),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/video_feed_tracking')
def video_feed_tracking():
    return Response(generate_tracking_frames(skip_frames=5, target_fps=5),
                    mimetype='multipart/x-mixed-replace; boundary=frame')

if __name__ == '__main__':
    app.run(debug=True, host="0.0.0.0", port=5000)
