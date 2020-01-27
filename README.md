# m5-stick-c-watch-demo

Demo for the M5Stick C watch, using AWS IoT to pub/sub.

From Arizona IoT Dev Fest 2020

[Full Instructions Here](https://github.com/aws-samples/aws-iot-esp32-arduino-examples/tree/master/m5stick-examples)

Play music on the watch speaker with cURL:
```
curl -d "{delay: 500, notes: [100, 200, 300]}" -H "Content-Type: application/json" -X POST <YOUR_ENDPOINT>
```
