graph LR
    %% Main Microcontroller
    ESP(("<b>ESP32 Module</b><br/>(Common Ground for all)"))

    %% Power Supply
    subgraph Power
        PWR["5V Power Source"] -- 5V --> ESP
        PWR -. GND .- ESP
    end

    %% Touch Controls
    subgraph Capacitive Touch
        T1(("Pad 1<br/>Buzzer")) --- P12["Pin 12"]
        T2(("Pad 2<br/>Display")) --- P4["Pin 4"]
        P12 --> ESP
        P4 --> ESP
    end

    %% E-Paper Display
    subgraph E-Paper Display Module
        ESP -- 3.3V --> EPD_VCC["VCC"]
        ESP -. GND .- EPD_GND["GND"]
        ESP -- Pin 13 --> EPD_DIN["DIN / MOSI"]
        ESP -- Pin 14 --> EPD_CLK["CLK / SCK"]
        ESP -- Pin 15 --> EPD_CS["CS"]
        ESP -- Pin 27 --> EPD_DC["DC"]
        ESP -- Pin 26 --> EPD_RST["RST"]
        ESP -- Pin 25 --> EPD_BUSY["BUSY"]
    end

    %% Analog Tank Inputs with Voltage Dividers
    subgraph Tank Sensors & Dividers
        direction LR
        S1["Tank 1 (Stbd Fuel)<br/>0-5V"] -->|47k| V1{"Node 1"} 
        V1 -->|82k| GND1["GND"]
        V1 -->|"0-3.17V"| P33["Pin 33"]

        S2["Tank 2 (Port Fuel)<br/>0-5V"] -->|47k| V2{"Node 2"} 
        V2 -->|82k| GND2["GND"]
        V2 -->|"0-3.17V"| P34["Pin 34"]

        S3["Tank 3 (Black Water)<br/>0-5V"] -->|47k| V3{"Node 3"} 
        V3 -->|82k| GND3["GND"]
        V3 -->|"0-3.17V"| P39["Pin 39"]

        S4["Tank 4 (Port Aft)<br/>0-5V"] -->|47k| V4{"Node 4"} 
        V4 -->|82k| GND4["GND"]
        V4 -->|"0-3.17V"| P32["Pin 32"]

        S5["Tank 5 (Stbd Water)<br/>0-5V"] -->|47k| V5{"Node 5"} 
        V5 -->|82k| GND5["GND"]
        V5 -->|"0-3.17V"| P35["Pin 35"]

        S6["Tank 6 (Port Fwd)<br/>0-5V"] -->|47k| V6{"Node 6"} 
        V6 -->|82k| GND6["GND"]
        V6 -->|"0-3.17V"| P36["Pin 36"]

        P33 --> ESP
        P34 --> ESP
        P39 --> ESP
        P32 --> ESP
        P35 --> ESP
        P36 --> ESP
    end

    %% Outputs (Buzzer & LEDs)
    subgraph Outputs
        ESP -- "Pin 19 (Active Low)" --> BUZ["Buzzer (-)"]
        PWR -- "5V / 3.3V" --> BUZ_V["Buzzer (+)"]

        ESP -- Pin 21 --> R1["220Ω"] --> LED1_R["LED 1 Red"]
        ESP -- Pin 22 --> R2["220Ω"] --> LED1_G["LED 1 Green"]
        ESP -- Pin 23 --> R3["220Ω"] --> LED1_B["LED 1 Blue"]

        ESP -- Pin 18 --> R4["220Ω"] --> LED2_R["LED 2 Red"]
        ESP -- Pin 16 --> R5["220Ω"] --> LED2_G["LED 2 Green"]
        ESP -- Pin 17 --> R6["220Ω"] --> LED2_B["LED 2 Blue"]
        
        ESP -- Pin 2 --> R7["330Ω"] --> STAT["Status LED"]
    end