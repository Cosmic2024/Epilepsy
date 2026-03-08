# TITLE : EPILEPSY

Standalone IoT-Based Seizure Detection System

 # Project Overview
This is a wearable MedTech solution designed for real-time detection of myoclonic seizures. It is built as a Standalone Environment, meaning it requires zero dependency on smartphones, Bluetooth tethering, or cloud servers to function. By processing data at the "Edge" on an ESP32, the system eliminates latency and ensures patient privacy.

# The "Truth Gate" Logic
To minimize false positives (a common demerit in existing apps), our system employs a multimodal detection gate: <br>
<ul> <b>1. Kinematic Data:</b> Uses a 3-axis Gyroscope (MPU6050) and dual-accelerometers (MPU6050 + ADXL345) to identify rhythmic involuntary jerks.</ul>
<ul><b>2. Physiological Data:</b>Integrates a Galvanic Skin Response (GSR) sensor to monitor spikes in skin conductance (sympathetic nervous activity) that typically accompany seizure events.</ul>
<ol><b>3. Thresholding:</b>An alarm is triggered only when both motion and GSR values cross research-backed thresholds simultaneously.</ol>

# Comparison Study

<table style="width:100%; border-collapse: collapse; font-family: Arial, sans-serif;">
  <thead>
    <tr style="background-color: #2c3e50; color: white; text-align: left;">
      <th style="padding: 12px; border: 1px solid #ddd;">Feature</th>
      <th style="padding: 12px; border: 1px solid #ddd;">Existing Apps (Apple/Samsung)</th>
      <th style="padding: 12px; border: 1px solid #ddd;">Our Solution</th>
    </tr>
  </thead>
  <tbody>
    <tr>
      <td style="padding: 12px; border: 1px solid #ddd; font-weight: bold;">Connectivity</td>
      <td style="padding: 12px; border: 1px solid #ddd;">Requires Smartphone/Bluetooth </td>
      <td style="padding: 12px; border: 1px solid #ddd; background-color: #ecfdf5;">Standalone (No Phone Needed) </td>
    </tr>
    <tr style="background-color: #f9f9f9;">
      <td style="padding: 12px; border: 1px solid #ddd; font-weight: bold;">Alert Type</td>
      <td style="padding: 12px; border: 1px solid #ddd;">Cloud-dependent (Latency) </td>
      <td style="padding: 12px; border: 1px solid #ddd; background-color: #ecfdf5;">Edge-based Local & SMS Alert </td>
    </tr>
    <tr>
      <td style="padding: 12px; border: 1px solid #ddd; font-weight: bold;">Sensors</td>
      <td style="padding: 12px; border: 1px solid #ddd;">Motion only (High False Alarms) </td>
      <td style="padding: 12px; border: 1px solid #ddd; background-color: #ecfdf5;">Motion + GSR (Truth Gate) </td>
    </tr>
    <tr style="background-color: #f9f9f9;">
      <td style="padding: 12px; border: 1px solid #ddd; font-weight: bold;">Availability</td>
      <td style="padding: 12px; border: 1px solid #ddd;">Subscription-based / High Cost</td>
      <td style="padding: 12px; border: 1px solid #ddd; background-color: #ecfdf5;">Low-cost & Open Accessible </td>
    </tr>
  </tbody>
</table>

<b> Many apps have phone-cloud integration but our app is a standalone with motion+GSR+Truth gate edge+local alarm. </b>

# The Simulation Model: 
<img width="947" height="750" alt="image" src="https://github.com/user-attachments/assets/bf7c1783-9e54-47eb-98fe-6d689337b7a5" />

<section style="font-family: Arial, sans-serif; line-height: 1.6; color: #333;">
  <h3 style="color: #2c3e50; border-bottom: 2px solid #eee; padding-bottom: 5px;">Scientific References</h3>
  <ul style="list-style-type: none; padding-left: 0;">
    <li style="margin-bottom: 12px; padding-left: 10px; border-left: 3px solid #3498db;">
      Jeppesen, J., et al. <strong>"Detection of generalized tonic–clonic seizures using wrist‐worn electrodermal activity and accelerometry."</strong> <em>Epilepsia</em> 63(4): 2022.
    </li>
    <li style="margin-bottom: 12px; padding-left: 10px; border-left: 3px solid #3498db;">
      Beniczky, S., et al. <strong>"Ultra–long-term seizure detection with wristband electrodermal activity and accelerometry."</strong> <em>Neurology</em> 97(15): 2021.
    </li>
    <li style="margin-bottom: 12px; padding-left: 10px; border-left: 3px solid #3498db;">
      Cogan, D., et al. <strong>"Automated detection of tonic seizures using wearable movement sensors and artificial neural networks."</strong> <em>Epilepsy Research</em> 198: 2024.
    </li>
    <li style="margin-bottom: 12px; padding-left: 10px; border-left: 3px solid #3498db;">
      Bruno, E., et al. <strong>"Reliable detection of generalized convulsive seizures using an off-the-shelf digital watch."</strong> <em>Epilepsia</em> 64(6): 2023.
    </li>
    <li style="margin-bottom: 12px; padding-left: 10px; border-left: 3px solid #3498db;">
      SeizeIT2 Consortium. <strong>"SeizeIT2: A large-scale dataset of multimodal wearable sensor data from epilepsy patients."</strong> <em>arXiv preprint</em> arXiv:2502.01224, 2025.
    </li>
    <li style="margin-bottom: 12px; padding-left: 10px; border-left: 3px solid #3498db;">
      Van Elmpt, W., et al. <strong>"Detection of nocturnal seizures with a wearable device: Combining accelerometry and electrodermal activity."</strong> <em>Seizure</em> 104: 2022.
    </li>
  </ul>
</section>



