# Normal-Cable
Rubber ducky hidden inside a standard cable
### Features:

**Very small**  
**Wifi + Bluetooth with antenna**  
**8MB flash**  
**USB Data and Power passthrough**  
**3.3V LDO**  

<img width="844" height="672" alt="image" src="https://github.com/user-attachments/assets/360b8cb4-9719-455c-b9df-45121118fcde" />  
<img width="927" height="594" alt="image" src="https://github.com/user-attachments/assets/a1f6917e-d0c6-454f-b41c-f5803734d566" />  
  
### **CAD must be slightly modified see instructions**  

<details>
<summary>Instructions</summary>
  
1. **Buy all parts:**  
   Either PCBA, or pcb with stencil and parts from lcsc, files for both options are found in production.  

2. **Modify CAD**  
   Get ruler and meassure diameter of the cable and make that hole at ends of both parts of case.  

3. **Solder cable pads:**  
   Solder all wires of cable to marked pads, same colour wires must be connected to same pads on both sides.   

4. **Assemble:**  
   Glue both parts together.  

5. **Flash**  
   
   Plug USB-C side and flash.  

   If needed RST is marked with silkscreen if needed:  
  
</details>

<details>
<summary>What has changed from v1</summary>
  
|V1|V2|
|-|-|
|Cheaper|Much more expensive|  
|4 layer PCB|6 layer PCB with blind and buried vias|  
|Bluetooth, Wifi|Bluetooth, Wifi|  
|50 Characters/sec|500 Characters/sec|  
|Max 100 Characters/sec|Max 1000 Characters/sec|  
|Bigger PCB|Smaller PCB|  
|Worse antenna range|Better antenna range|  
|No ESD protection|Good ESD protection|  
|ESP-32 C6|ESP-32 S3 With native USB|  

**max characters a second is the theoretical limit as each key needs pressed and released event so half of that is the actual speed.**   

</details>

<details>
<summary>Pictures</summary>
  
**Schematic:**  
<img width="965" height="811" alt="image" src="https://github.com/user-attachments/assets/f462a83e-e455-4ce3-9601-7bcfdbb9efe6" />  

**PCB:**  
<img width="289" height="785" alt="image" src="https://github.com/user-attachments/assets/375b36c5-e000-4ee9-8e22-75599ae8cc38" />

**CAD:**  
<img width="896" height="451" alt="image" src="https://github.com/user-attachments/assets/4adf267b-bd2f-4b89-ac0e-04849215f0cf" />

**CAD:**  
<img width="644" height="529" alt="image" src="https://github.com/user-attachments/assets/76722541-5666-46c9-ba51-b89651aadb54" />


</details>

<details>
<summary>BOM</summary>
  
| Name | Purpose | Quantity | Total Cost (USD) | Link | Distributor |
|------|---------|----------|-------------------|------|-------------|
| PCB | all components are on this | 5 | 146.76 | [pcbway.com](https://pcbway.com) | PCBway |
| ALL components | all components that are on pcb | 1 | 27.69 | [lcsc.com](https://lcsc.com) | lcsc |
| Stencil | preciselly spreads solder paste | 1 | 10 | [pcbway.com](https://pcbway.com) | PCBway |
| Cable | Cable that connects both PCBs | 1 | 8 | any usb 2.0 cable works | Any |
| | | **Total cost** | **192.45** | | |
  
</details>




