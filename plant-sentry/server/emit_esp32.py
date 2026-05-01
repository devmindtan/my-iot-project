import os
from flask import Flask, request
from supabase import create_client, Client
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)

supabase: Client = create_client(
    os.environ.get("SUPABASE_URL"),
    os.environ.get("SUPABASE_KEY")
)

@app.route('/')
def index():
    response = supabase.table('automation_rules').select("*").execute()
    test = response.data

    html = '<h1>Good</h1><ul>'
    for item in test:
        html += f'<li>{item["name"]}</li>'
    html += '</ul>'

    return html

@app.route('/data', methods=['POST'])
def receive_data():
    data = request.json

    supabase.table("sensor_data").insert({
        "device_id": data.get("device_id"),
        "temperature": data.get("temperature"),
        "moisture": data.get("moisture")
    }).execute()

    return {"status": "ok"}


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)