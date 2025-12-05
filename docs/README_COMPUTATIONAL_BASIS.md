# BioSimulator Pro - Fundamentos Computacionales

## Información del Documento
| Campo | Valor |
|-------|-------|
| **Versión** | 1.0.0 |
| **Propósito** | Explicar cómo el software construye las señales ECG, EMG y PPG paso a paso |

---

## Índice

### Parte I: Modelo ECG
1. [La Idea Central: Un Punto que Gira](#1-la-idea-central-un-punto-que-gira)
2. [Las Cinco Ondas del ECG](#2-las-cinco-ondas-del-ecg)
3. [Construcción Paso a Paso de la Señal ECG](#3-construcción-paso-a-paso-de-la-señal)
4. [Cómo se Calcula Cada Muestra ECG](#4-cómo-se-calcula-cada-muestra)
5. [Haciendo que Cada Latido sea Diferente](#5-haciendo-que-cada-latido-sea-diferente)
6. [De Números a Voltaje Real (ECG)](#6-de-números-a-voltaje-real)

### Parte II: Modelo EMG
7. [EMG: La Orquesta de Tambores](#7-emg-la-orquesta-de-tambores)
8. [Cómo Funciona una Unidad Motora](#8-cómo-funciona-una-unidad-motora)
9. [El Reclutamiento: Activando los Tambores](#9-el-reclutamiento-activando-los-tambores)
10. [Generando la Señal EMG](#10-generando-la-señal-emg)
11. [Condiciones Especiales del EMG](#11-condiciones-especiales-del-emg)
12. [De Números a Voltaje Real (EMG)](#12-de-números-a-voltaje-real-emg)

### Parte III: Modelo PPG
13. [PPG: El Pulso de Luz](#13-ppg-el-pulso-de-luz)
14. [Anatomía de un Pulso PPG](#14-anatomía-de-un-pulso-ppg)
15. [Construyendo el Pulso con Gaussianas](#15-construyendo-el-pulso-con-gaussianas)
16. [Condiciones Clínicas del PPG](#16-condiciones-clínicas-del-ppg)
17. [De Números a Voltaje Real (PPG)](#17-de-números-a-voltaje-real-ppg)

### Parte IV: Diagramas de Flujo
18. [Diagrama de Flujo: Modelo ECG](#18-diagrama-de-flujo-modelo-ecg)
19. [Diagrama de Flujo: Modelo EMG](#19-diagrama-de-flujo-modelo-emg)
20. [Diagrama de Flujo: Modelo PPG](#20-diagrama-de-flujo-modelo-ppg)

---

## 1. La Idea Central: Un Punto que Gira

### 1.1 El Concepto Fundamental

Imagina un reloj analógico. La manecilla de los segundos da vueltas continuamente alrededor del centro. En nuestro modelo ECG, tenemos algo muy similar: **un punto imaginario que rota alrededor de un círculo**.

Este punto tiene una posición `(x, y)` que cambia constantemente. Cuando el punto completa una vuelta completa alrededor del círculo, eso representa **un latido cardíaco completo**.

La velocidad a la que gira el punto determina la frecuencia cardíaca:
- Si gira rápido → frecuencia cardíaca alta (taquicardia)
- Si gira lento → frecuencia cardíaca baja (bradicardia)

### 1.2 El Ángulo como Posición en el Ciclo Cardíaco

En cualquier momento, podemos saber "dónde estamos" en el ciclo cardíaco mirando el ángulo del punto. Llamamos a este ángulo **theta (θ)**:

- Cuando θ = 0° estamos en el **pico de la onda R** (el pico más alto del ECG)
- Cuando θ = -70° estamos en la **onda P**
- Cuando θ = +100° estamos en la **onda T**

Es como si el ciclo cardíaco fuera un pastel circular dividido en sectores, y cada sector corresponde a una parte diferente del latido.

### 1.3 La Tercera Dimensión: La Señal ECG

Además de las coordenadas `(x, y)` del punto rotando, tenemos una tercera variable llamada **z**. Esta variable `z` es la que realmente nos importa porque **es el valor del ECG que vemos en pantalla**.

Mientras el punto gira en el plano `(x, y)`, el valor de `z` sube y baja formando las famosas ondas P, Q, R, S y T del electrocardiograma.

---

## 2. Las Cinco Ondas del ECG

### 2.1 Anatomía de un Latido

Cada latido del corazón produce una secuencia característica de ondas en el electrocardiograma. Estas ondas tienen nombres de letras: **P, Q, R, S y T**. Cada una representa un evento eléctrico diferente en el corazón:

| Onda | ¿Qué representa? | ¿Cómo se ve? |
|------|------------------|--------------|
| **P** | La contracción de las aurículas (las cámaras superiores del corazón) | Una pequeña joroba hacia arriba |
| **Q** | El inicio de la contracción de los ventrículos | Un pequeño pico hacia abajo |
| **R** | La contracción principal de los ventrículos | El pico más alto y prominente |
| **S** | El final de la contracción ventricular | Otro pico hacia abajo |
| **T** | La relajación de los ventrículos (repolarización) | Una joroba ancha hacia arriba |

### 2.2 Cómo el Modelo Define Cada Onda

Para generar cada onda, el modelo necesita saber tres cosas sobre ella:

**1. ¿DÓNDE aparece la onda?** (posición en el ciclo)

Cada onda tiene una posición angular fija. Usamos el pico R como referencia (ángulo cero):
- La onda P aparece **70 grados antes** del pico R
- La onda Q aparece **15 grados antes** del pico R
- La onda R aparece **exactamente en el ángulo cero** (es nuestra referencia)
- La onda S aparece **15 grados después** del pico R
- La onda T aparece **100 grados después** del pico R

**2. ¿QUÉ TAN ALTA es la onda?** (amplitud)

Cada onda tiene una altura característica. Los números son relativos entre sí:
- La onda P tiene amplitud **1.2** (pequeña, hacia arriba)
- La onda Q tiene amplitud **-5.0** (pequeña, hacia abajo - el signo negativo indica dirección)
- La onda R tiene amplitud **30.0** (¡la más grande! hacia arriba)
- La onda S tiene amplitud **-7.5** (moderada, hacia abajo)
- La onda T tiene amplitud **0.75** (moderada, hacia arriba)

**3. ¿QUÉ TAN ANCHA es la onda?** (duración)

Cada onda tiene un ancho característico:
- La onda P es **relativamente ancha** (0.25) porque la despolarización auricular es lenta
- Las ondas Q, R y S son **estrechas** (0.10) porque ocurren rápidamente
- La onda T es **la más ancha** (0.40) porque la repolarización es un proceso gradual

### 2.3 Visualización de la Secuencia

```
         R ← El pico más alto (amplitud 30)
         │
         █
         █
    P    █         T ← Onda ancha de repolarización
   ┌─┐   █       ┌───┐
   │ │   █       │   │
───┴─┴───█───────┴───┴─── línea base
       Q █ S
         └┘
         ↑
    Ondas negativas (van hacia abajo)
```

---

## 3. Construcción Paso a Paso de la Señal

### 3.1 El Proceso de Generación

Imagina que eres un artista dibujando el trazo del ECG en un papel que se mueve constantemente hacia la izquierda. Tu lápiz sube y baja creando la forma característica. El modelo hace exactamente esto, pero matemáticamente.

El proceso ocurre **1000 veces por segundo** (frecuencia de muestreo de 1 kHz). Cada milisegundo, el sistema:

1. **Avanza el punto rotatorio** un poquito en su órbita circular
2. **Calcula qué tan cerca está** de cada una de las cinco ondas
3. **Suma las contribuciones** de todas las ondas cercanas
4. **Produce un valor de salida** que representa el voltaje del ECG en ese instante

### 3.2 La Magia de las Campanas Gaussianas

Cada onda del ECG se genera usando una forma matemática llamada **campana de Gauss** (la famosa "curva de campana" de las estadísticas). 

¿Por qué campanas? Porque una campana de Gauss tiene exactamente las propiedades que necesitamos:
- Tiene un **pico central** definido
- Se desvanece **suavemente** hacia los lados
- Podemos controlar su **altura** (qué tan alto es el pico)
- Podemos controlar su **anchura** (qué tan rápido se desvanece)

Cuando el punto rotatorio pasa cerca de la posición de una onda (digamos, la onda R en ángulo 0°), esa campana gaussiana "se activa" y contribuye al valor de salida. Mientras más cerca esté el punto del centro de la campana, mayor es la contribución.

### 3.3 La Construcción del Valor ECG

En cada instante, el valor del ECG se construye sumando las contribuciones de las cinco ondas:

**Paso 1: ¿Dónde estamos?**
Primero calculamos el ángulo actual (theta) del punto rotatorio. Este ángulo nos dice en qué parte del ciclo cardíaco estamos.

**Paso 2: ¿Qué tan lejos estamos de cada onda?**
Para cada una de las cinco ondas, calculamos la distancia angular entre nuestra posición actual y el centro de esa onda. Por ejemplo, si estamos en ángulo -10° y la onda Q está en -15°, la distancia es solo 5°.

**Paso 3: ¿Cuánto contribuye cada onda?**
Las ondas cercanas contribuyen mucho, las lejanas contribuyen casi nada. La campana gaussiana se encarga de esto automáticamente:
- Si la distancia es pequeña → contribución grande
- Si la distancia es grande → contribución casi cero

**Paso 4: Sumar todo**
Sumamos las contribuciones de las cinco ondas. El resultado es el valor del ECG en ese instante.

**Paso 5: Volver hacia la línea base**
Además, hay una fuerza que constantemente "empuja" el valor de vuelta hacia la línea base (el nivel cero). Esto evita que la señal se vaya hacia arriba o abajo indefinidamente.

### 3.4 Ejemplo Narrativo de un Ciclo Completo

Imaginemos un ciclo cardíaco completo, contado como una historia:

**Momento 1: Comenzando en la onda P (θ ≈ -70°)**
El punto rotatorio está pasando por la región de la onda P. La campana gaussiana de la onda P está "activa", así que el valor de z sube un poquito, creando la pequeña joroba de la onda P.

**Momento 2: Descendiendo hacia Q (θ ≈ -15°)**
El punto se acerca a la onda Q. Como Q tiene amplitud negativa, el valor de z baja brevemente, creando la pequeña deflexión negativa.

**Momento 3: ¡El pico R! (θ = 0°)**
Aquí es donde ocurre la magia. La onda R tiene la amplitud más grande (30.0), así que cuando el punto pasa por θ = 0°, el valor de z sube dramáticamente, creando el característico pico alto del ECG.

**Momento 4: Cayendo hacia S (θ ≈ +15°)**
Inmediatamente después del pico R, la onda S (negativa) toma el control y el valor cae por debajo de la línea base.

**Momento 5: El segmento ST (θ ≈ +20° a +80°)**
Aquí hay un período de calma relativa. No hay ondas principales, así que el valor regresa hacia la línea base y se mantiene relativamente plano. Este es el famoso "segmento ST" que los cardiólogos observan para detectar infartos.

**Momento 6: La onda T (θ ≈ +100°)**
Finalmente, la onda T crea una elevación suave y ancha mientras el corazón se "recarga" (repolarización ventricular).

**Momento 7: Vuelta al inicio (θ → siguiente ciclo)**
El punto completa su órbita y el ciclo comienza de nuevo con la siguiente onda P.

---

## 4. Cómo se Calcula Cada Muestra

### 4.1 El Problema del Movimiento Continuo

El corazón no late en "saltos" discretos, sino que genera una señal eléctrica continua y suave. Sin embargo, las computadoras trabajan con números discretos. Entonces, ¿cómo simulamos algo continuo con una máquina digital?

La respuesta es **calcular muchas muestras muy rápido**. Si calculamos 1000 valores por segundo (uno cada milisegundo), y los conectamos suavemente, el resultado parece continuo para nuestros ojos y para cualquier equipo que analice la señal.

### 4.2 El Método de los Cuatro Pasos (Runge-Kutta)

Para calcular cómo cambian nuestras tres variables (x, y, z) de un instante al siguiente, usamos un método llamado **Runge-Kutta de cuarto orden**, o simplemente "RK4". 

¿Por qué no simplemente sumar el cambio directamente? Porque eso introduce errores. El método RK4 es más inteligente: en lugar de dar un solo paso grande, da **cuatro pasos de prueba** y luego combina la información para dar un paso final más preciso.

Piénsalo como cruzar un río saltando entre piedras:
- **Método simple:** Miras la piedra más cercana, saltas, y repites. Podrías terminar en un camino malo.
- **Método RK4:** Miras adelante, evalúas varias opciones, consideras cómo cambia el terreno, y luego haces un salto óptimo que te lleva por el mejor camino.

### 4.3 Los Cuatro Pasos Explicados

Imagina que estás en el punto A y quieres llegar al punto B (un milisegundo después):

**Primer paso (k1):** "Si sigo en la dirección actual, ¿hacia dónde voy?"
Calculamos la velocidad de cambio en nuestra posición actual.

**Segundo paso (k2):** "Si me muevo medio camino usando k1, ¿cómo cambia la dirección?"
Damos un paso a la mitad del intervalo usando k1, y recalculamos la velocidad desde ahí.

**Tercer paso (k3):** "¿Y si uso la nueva dirección k2 para ir a la mitad?"
Similar a k2, pero usando la información mejorada.

**Cuarto paso (k4):** "Si doy el paso completo con k3, ¿cuál es la velocidad final?"
Vamos hasta el final del intervalo y medimos la velocidad allí.

**Combinación final:** Promediamos las cuatro velocidades, dando más peso a las del medio (k2 y k3), y usamos ese promedio para dar el paso real. El resultado es mucho más preciso que cualquier método simple.

### 4.4 ¿Por Qué Importa la Precisión?

Si usáramos un método menos preciso, los errores se acumularían con el tiempo. Después de miles de muestras, la señal se degradaría:
- Las ondas podrían distorsionarse
- El ritmo podría desviarse
- La forma característica del ECG se perdería

Con RK4, los errores se mantienen tan pequeños que la señal permanece limpia y estable incluso después de horas de operación continua.

### 4.5 Las Tres Ecuaciones del Sistema

El modelo tiene tres ecuaciones que se calculan simultáneamente:

**Ecuación para X:** Describe cómo cambia la coordenada horizontal del punto rotatorio. Incluye un término que lo mantiene cerca del círculo unitario y otro que lo hace girar.

**Ecuación para Y:** Similar a X, describe el movimiento vertical del punto. Junto con X, estas dos ecuaciones hacen que el punto trace un círculo.

**Ecuación para Z:** Esta es la más importante. Calcula el valor del ECG sumando las contribuciones de las cinco ondas y restando un término que lo devuelve a la línea base.

---

## 5. Haciendo que Cada Latido sea Diferente

### 5.1 El Problema del ECG "Demasiado Perfecto"

Si simplemente repetimos el mismo patrón una y otra vez, el ECG resultante se ve artificial. Un cardiólogo experimentado lo notaría inmediatamente: "Esto parece generado por computadora".

¿Por qué? Porque en un corazón real, **ningún latido es exactamente igual al anterior**. Hay pequeñas variaciones naturales causadas por:

- **La respiración:** Cuando inhalas, el corazón se mueve ligeramente dentro del pecho, cambiando las amplitudes de las ondas
- **El sistema nervioso:** El nervio vago modula constantemente el ritmo cardíaco
- **Pequeñas fluctuaciones eléctricas:** El sistema de conducción del corazón no es perfectamente constante

### 5.2 Variabilidad del Ritmo Cardíaco (HRV)

La variación más importante es la del **intervalo RR** (el tiempo entre latidos). En una persona sana en reposo, este intervalo NO es constante. Si el corazón late a 75 BPM en promedio, el intervalo medio es 0.8 segundos, pero cada latido individual podría ser:

- 0.76 segundos
- 0.82 segundos  
- 0.79 segundos
- 0.84 segundos
- ...y así sucesivamente

Esta variación es completamente **normal y saludable**. De hecho, un corazón con ritmo "demasiado constante" puede indicar problemas. Los estudios científicos (Task Force 1996) documentan que una variación del 5-8% es típica en reposo.

### 5.3 Cómo Implementamos la Variabilidad

Cada vez que termina un latido y comienza el siguiente, el sistema:

**1. Genera un nuevo intervalo RR**

El próximo intervalo no es simplemente el promedio. Se calcula tomando el valor promedio y añadiendo una variación aleatoria. Esta variación sigue una distribución de campana (Gaussiana), lo que significa que:
- Los valores cercanos al promedio son más probables
- Los valores muy alejados son raros pero posibles
- El resultado es natural y realista

**2. Modifica ligeramente la morfología**

Además del ritmo, cada latido tiene pequeñas variaciones en la forma de las ondas:

| Qué varía | Cuánto varía | ¿Por qué? |
|-----------|--------------|-----------|
| Amplitudes de las ondas | ±5% | Simula el efecto de la respiración sobre el vector cardíaco |
| Anchos de las ondas | ±2% | Pequeñas variaciones en la velocidad de conducción |
| Posiciones de las ondas | ±0.6° | Timing sutil entre las ondas |

**3. Añade una deriva lenta de la línea base**

El "cero" del ECG no es perfectamente estable. Hay una oscilación muy lenta (aproximadamente cada 4 segundos) que simula el movimiento del pecho con la respiración. Esta deriva es muy pequeña (unos 15 microvoltios) pero añade realismo.

### 5.4 Variabilidad en Diferentes Condiciones

No todas las condiciones cardíacas tienen la misma variabilidad:

| Condición | Variabilidad RR | Explicación |
|-----------|-----------------|-------------|
| **Normal** | 5-8% | Modulación vagal saludable |
| **Taquicardia** | 3-6% | Menos variabilidad a frecuencias altas |
| **Bradicardia** | 2-5% | Puede indicar bloqueo o atleta entrenado |
| **Fibrilación Auricular** | 15-35% | ¡Extremadamente irregular! Característica diagnóstica |

La fibrilación auricular es especialmente interesante porque su "irregularmente irregular" es tan distintivo que es diagnóstico por sí solo.

### 5.5 Números Aleatorios con Forma de Campana

Para generar las variaciones, necesitamos números aleatorios que sigan una distribución Gaussiana (de campana). El ESP32 puede generar números aleatorios uniformes (todos igualmente probables), pero nosotros necesitamos que los valores cercanos al promedio sean más frecuentes.

Usamos una técnica llamada **Box-Muller** que transforma pares de números uniformes en números Gaussianos. El resultado son variaciones que:
- Casi siempre están cerca del valor deseado
- Ocasionalmente se alejan un poco
- Muy raramente se alejan mucho
- Exactamente como ocurre en la naturaleza

---

## 6. De Números a Voltaje Real

### 6.1 El Viaje de un Número

Hasta ahora hemos hablado de números abstractos. Pero al final del día, necesitamos generar un **voltaje real** que salga por un pin del microcontrolador. ¿Cómo convertimos nuestros cálculos matemáticos en electricidad?

### 6.2 Definiendo una Escala Significativa

Primero, decidimos que nuestros números van a representar **milivolts (mV)**. Esta es la unidad estándar en electrocardiografía.

En un ECG real de Lead II (la derivación más común):
- La onda R típica mide entre 0.5 y 1.5 mV
- La onda P mide unos 0.1 a 0.25 mV
- La onda T mide unos 0.1 a 0.5 mV
- Las ondas Q y S son negativas, entre -0.1 y -0.5 mV

Con nuestros parámetros, el modelo produce valores en un rango similar, así que definimos que **1.0 en el modelo equivale a 1.0 mV**. Esta equivalencia hace que los números tengan significado clínico.

### 6.3 El Convertidor Digital-Analógico (DAC)

El ESP32 tiene un componente llamado DAC (Digital-to-Analog Converter) que puede convertir un número digital en un voltaje real. El DAC del ESP32:
- Acepta valores de **0 a 255** (8 bits)
- Produce voltajes de **0 a 3.3 volts**

Ahora tenemos un problema de traducción: nuestro ECG está en milivolts (digamos, de -0.5 a +1.5 mV) pero el DAC solo entiende números del 0 al 255.

### 6.4 El Mapeo Lineal

Usamos una conversión lineal simple. Definimos que:
- **-0.5 mV** (el valor más bajo que esperamos) corresponde a **DAC = 0** → **0.0 volts de salida**
- **+1.5 mV** (el valor más alto que esperamos) corresponde a **DAC = 255** → **3.3 volts de salida**

Todo lo intermedio se mapea proporcionalmente:

| Valor ECG | Valor DAC | Voltaje de Salida | ¿Qué representa? |
|-----------|-----------|-------------------|------------------|
| -0.5 mV | 0 | 0.00 V | Onda Q o S muy profunda |
| 0.0 mV | 64 | 0.83 V | Línea base (isoeléctrica) |
| +0.5 mV | 127 | 1.65 V | Onda T típica |
| +1.0 mV | 191 | 2.47 V | Pico R normal |
| +1.5 mV | 255 | 3.30 V | Pico R muy alto |

### 6.5 La Fórmula de Conversión

Para convertir de milivolts a valor DAC, el cálculo es:

1. **Normalizar:** Tomar el valor en mV, sumarle 0.5 (para que el mínimo sea cero), y dividir entre 2.0 (el rango total). Esto da un número entre 0 y 1.

2. **Escalar:** Multiplicar por 255 para obtener el valor DAC.

Por ejemplo, si tenemos +1.0 mV:
- Sumamos 0.5 → 1.5
- Dividimos entre 2.0 → 0.75
- Multiplicamos por 255 → 191.25 ≈ 191

### 6.6 Interpretación con Osciloscopio

Si conectas un osciloscopio a la salida del ESP32, verás la señal ECG. Para interpretar los valores:

- **Factor de escala:** Cada 1 mV del ECG simulado corresponde a 1.65 volts en la salida
- **Offset:** La línea base (0 mV) aparece a 0.83 volts

Esto significa que si ves un pico de 2.5 volts en el osciloscopio, ese pico representa aproximadamente 1.0 mV en términos de ECG. El rango visual completo (0 a 3.3V) cubre un rango de ECG de 2 mV (-0.5 a +1.5).

---

## 7. Resumen: El Flujo Completo del ECG

### 7.1 Vista General del Proceso

Todo el proceso puede resumirse en estos pasos que ocurren 1000 veces por segundo:

**1. El punto rota**
Un punto imaginario gira alrededor de un círculo. Su ángulo nos dice en qué parte del ciclo cardíaco estamos.

**2. Las ondas contribuyen**
Cinco "campanas" gaussianas (P, Q, R, S, T), cada una en su posición angular, contribuyen al valor de salida según qué tan cerca está el punto rotatorio.

**3. Las ecuaciones se integran**
El método RK4 calcula cómo cambian las tres variables (x, y, z) en el siguiente milisegundo.

**4. Se detectan los latidos**
Cuando el punto cruza el ángulo cero, hemos completado un latido. En ese momento se aplican las variaciones para el próximo latido.

**5. El valor se convierte**
El número z (en mV equivalentes) se mapea a un valor DAC (0-255) que produce un voltaje real en el pin de salida.

### 7.2 Lo que hace especial a este modelo

- **Base científica:** El modelo McSharry está publicado y validado
- **Variabilidad natural:** Cada latido es ligeramente diferente
- **Patologías realistas:** Las condiciones cardíacas alteran los parámetros de formas clínicamente correctas
- **Eficiencia:** El ESP32 puede generar la señal en tiempo real sin problemas

---

# Parte II: Modelo EMG

---

## 7. EMG: La Orquesta de Tambores

### 7.1 La Diferencia Fundamental entre ECG y EMG

Mientras que el ECG es como un **solo tambor** que late rítmicamente (el corazón), el EMG es como una **orquesta de 100 tambores** donde cada uno toca a su propio ritmo.

| ECG | EMG |
|-----|-----|
| Una fuente (corazón) | 100 fuentes (unidades motoras) |
| Señal periódica (se repite) | Señal estocástica (aleatoria) |
| Forma fija (PQRST) | Sin forma fija (interferencia) |
| Puedes ver cada latido | No puedes distinguir cada disparo |

### 7.2 ¿Por qué el EMG se ve como "ruido"?

Imagina una sala con 100 personas aplaudiendo. Si todos aplauden al mismo tiempo, escucharías golpes claros y separados. Pero si cada persona aplaude a su propio ritmo, escucharías un ruido constante donde no puedes distinguir cada aplauso individual.

Eso es exactamente lo que pasa en el EMG:
- Cada **unidad motora** es como una persona aplaudiendo
- Cada **disparo** es como un aplauso
- Cuando muchas disparan a diferentes tiempos, la suma es un **ruido estructurado**

### 7.3 El Resultado Visual

```
ECG (un tambor rítmico):
    R         R         R         R
    █         █         █         █
   ╱ ╲       ╱ ╲       ╱ ╲       ╱ ╲
──P───T───P───T───P───T───P───T──
  └──────┘ └──────┘ └──────┘
   1 latido  1 latido  1 latido

EMG (100 tambores aleatorios):
▄█▅▃▂▁▃▄█▅▃▂▁▃▄▆█▅▃▂▁▃▄▅▇█▅▃▂▁▂▄▅
────────────────────────────────────
No hay patrón repetitivo identificable
```

---

## 8. Cómo Funciona una Unidad Motora

### 8.1 Anatomía de una Unidad Motora

Una **unidad motora (MU)** es la unidad básica del control muscular. Consiste en:

- **Una motoneurona:** La célula nerviosa que envía la señal desde la médula espinal
- **Fibras musculares:** Todas las fibras que esa motoneurona controla (pueden ser 10 a 2000)

Cuando la motoneurona "dispara", **todas** sus fibras se contraen simultáneamente. Es un sistema de "todo o nada".

### 8.2 El Principio de Henneman: Pequeños Primero

En 1965, Elwood Henneman descubrió que las unidades motoras siempre se reclutan en el mismo orden:

**Las pequeñas primero, las grandes después.**

| Orden | Tipo de MU | Características |
|-------|------------|-----------------|
| Primero | Pequeñas (Tipo I) | Pocas fibras, resistentes a fatiga, control fino |
| Después | Medianas | Balance entre control y fuerza |
| Último | Grandes (Tipo II) | Muchas fibras, fatigables, mucha fuerza |

Esto tiene sentido: para mover una taza de café no necesitas la misma fuerza que para levantar una pesa. Tu cuerpo activa solo las MUs necesarias.

### 8.3 Cada MU Tiene su Personalidad

En nuestro modelo, cada una de las 100 MUs tiene propiedades únicas:

**1. Umbral de reclutamiento:** ¿Cuánta "señal del cerebro" se necesita para activarla?
- Las primeras MUs se activan con solo 2-5% de esfuerzo
- Las últimas MUs requieren 60% de esfuerzo máximo

**2. Amplitud del MUAP:** ¿Qué tan grande es su señal eléctrica?
- MUs pequeñas: ~0.3 mV
- MUs grandes: ~1.5 mV

**3. Frecuencia de disparo:** ¿Qué tan rápido puede "disparar"?
- Al reclutar: 6-8 Hz (disparos por segundo)
- Con más esfuerzo: hasta 50 Hz

---

## 9. El Reclutamiento: Activando los Tambores

### 9.1 La Señal del Cerebro: Excitación

Cuando decides mover un músculo, tu cerebro envía una señal que llamamos **excitación**. Esta señal va de 0 (reposo total) a 1 (contracción máxima).

A medida que aumentas la excitación:
1. Primero se activan las MUs de umbral bajo (las pequeñas)
2. Luego se van sumando las de umbral más alto
3. Las MUs ya activas aumentan su frecuencia de disparo

### 9.2 Visualización del Reclutamiento

```
Excitación 10%:
MU 1-10 activas, resto dormidas
▂▃▂▁ ▂▃▂▁ ▂▃▂▁     (pocas señales, espaciadas)

Excitación 30%:
MU 1-30 activas
▃▄▅▃▂▁▂▃▄▅▃▂▁▂▃▄▅  (más señales, empezando a superponerse)

Excitación 60%:
MU 1-100 TODAS activas
▄█▆▄▃▅█▇▅▃▄▆█▅▄▃▅█  (patrón de interferencia completo)

Excitación 100%:
Todas las MUs a máxima frecuencia
▆█▇█▆▇█▇▆█▇█▆▇█▇█▆  (interferencia muy densa)
```

### 9.3 ¿Por Qué Solo el 60%?

Un dato interesante: todas las MUs se reclutan aproximadamente al 60% de la excitación. ¿Entonces qué pasa del 60% al 100%?

**La frecuencia de disparo aumenta.**

Una MU ya reclutada puede aumentar su frecuencia de 8 Hz a 50 Hz. Esto genera más fuerza sin necesidad de reclutar más MUs (porque ya están todas activas).

---

## 10. Generando la Señal EMG

### 10.1 El Proceso Cada Milisegundo

El sistema realiza estos pasos 1000 veces por segundo:

**Paso 1: ¿Cuál es la excitación actual?**
Según la condición (reposo, leve, moderada, etc.), se establece el nivel de excitación. También se añade variabilidad natural (±4% oscilando a 2 Hz).

**Paso 2: ¿Qué MUs están activas?**
Para cada una de las 100 MUs, verificamos si la excitación actual supera su umbral. Si sí, la MU está activa.

**Paso 3: ¿Cuál es la frecuencia de cada MU activa?**
Calculamos la frecuencia de disparo basada en qué tan por encima del umbral está la excitación.

**Paso 4: ¿Es tiempo de que dispare alguna MU?**
Cada MU tiene programado cuándo será su próximo disparo. Si el tiempo actual llegó a ese momento, la MU dispara.

**Paso 5: Sumar todas las contribuciones**
Por cada MU que disparó recientemente, añadimos su MUAP (la forma de la señal eléctrica) a la salida.

**Paso 6: Añadir ruido**
Un poco de ruido de fondo simula interferencias del electrodo y del ambiente.

### 10.2 La Forma del MUAP: El Sombrero Mexicano

Cuando una unidad motora dispara, produce un potencial eléctrico llamado MUAP (Motor Unit Action Potential). Este tiene una forma característica de **tres fases**:

```
      ╭──╮              ╭──╮
     ╱    ╲            ╱    ╲     ← Fases positivas pequeñas
────╯      ╲          ╱      ╰────
            ╲        ╱
             ╲──────╯               ← Fase negativa grande
```

Esta forma se llama **"Mexican Hat Wavelet"** (sombrero mexicano) porque, vista de perfil, parece un sombrero con ala y copa invertida.

¿Por qué esta forma? Porque la señal eléctrica primero se acerca al electrodo (positivo pequeño), pasa justo debajo (negativo grande), y luego se aleja (positivo pequeño).

### 10.3 La Variabilidad del Disparo

En la realidad, una MU no dispara con precisión de reloj. Hay variabilidad natural de ±20% en el tiempo entre disparos. Esto es importante porque:

- Si todas las MUs dispararan como relojes, el EMG sonaría artificial
- La variabilidad da el carácter "natural" y ruidoso del EMG real

---

## 11. Condiciones Especiales del EMG

### 11.1 Las 10 Condiciones

El modelo puede simular estas condiciones:

| Condición | Excitación | Comportamiento |
|-----------|------------|----------------|
| **REST** | 0% | Solo ruido, ninguna MU activa |
| **MILD** | 20% | Pocas MUs, disparos identificables |
| **MODERATE** | 50% | Interferencia parcial |
| **STRONG** | 80% | Interferencia densa |
| **MAXIMUM** | 100% | Máxima actividad |
| **TREMOR** | Oscila 0-60% | Patrón rítmico a 5 Hz |
| **MYOPATHY** | 40% | MUAPs pequeños (enfermedad muscular) |
| **NEUROPATHY** | 50% | MUAPs gigantes (pérdida de nervios) |
| **FASCICULATION** | 0% | Disparos aleatorios espontáneos |
| **FATIGUE** | 60%→90% | Degradación progresiva |

### 11.2 Miopatía: Músculo Enfermo

En una miopatía (como distrofia muscular), las fibras musculares están dañadas. Cada unidad motora tiene menos fibras funcionales, así que su MUAP es más pequeño:

```
Normal:          Miopatía:
    █                ▄
   ╱ ╲              ╱ ╲
──╯   ╰──        ──╯   ╰──
Amplitud normal  Amplitud 40%
```

El paciente compensa reclutando más MUs, pero la fuerza sigue siendo menor.

### 11.3 Neuropatía: Nervios Dañados

En una neuropatía, algunas motoneuronas mueren. Pero las fibras huérfanas no quedan inactivas: son "adoptadas" por las MUs vecinas. El resultado son MUAPs **gigantes**:

```
Normal:          Neuropatía:
    █                ████
   ╱ ╲              ██████
──╯   ╰──        ──╯      ╰──
Amplitud normal  Amplitud 250%
```

Menos MUs, pero cada una controla más fibras. Patrón de reclutamiento reducido.

### 11.4 Temblor Parkinsoniano

En el temblor de Parkinson, la señal del cerebro oscila involuntariamente a 4-6 Hz:

```
Tiempo →
Excitación:  ∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿∿  (onda sinusoidal 5 Hz)
EMG:         ▄█▂ ▄█▂ ▄█▂ ▄█▂ ▄█▂  (activación rítmica)
              └─┘ └─┘ └─┘ └─┘
            200ms por ciclo (5 Hz)
```

### 11.5 Fatiga

Durante fatiga sostenida:
1. Las frecuencias de disparo disminuyen (hasta 30%)
2. La variabilidad aumenta
3. Se reclutan más MUs para mantener la fuerza
4. El RMS puede aumentar mientras la fuerza cae

---

## 12. De Números a Voltaje Real (EMG)

### 12.1 Rango de Amplitudes

El EMG de superficie tiene amplitudes mucho menores que el ECG:

| Señal | Rango típico |
|-------|--------------|
| ECG | 0.5 - 2 mV (pico R) |
| EMG normal | 0.1 - 2 mV RMS |
| EMG neuropático | hasta 5 mV (MUAPs gigantes) |

Por eso usamos un rango de **±5 mV** para el EMG, permitiendo ver tanto señales normales como los MUAPs gigantes de neuropatía.

### 12.2 Conversión a DAC

El mapeo es diferente al ECG:

| Valor EMG | Valor DAC | Voltaje Salida |
|-----------|-----------|----------------|
| −5.0 mV | 0 | 0.00 V |
| 0.0 mV | 128 | 1.65 V |
| +5.0 mV | 255 | 3.30 V |

**Línea base:** A diferencia del ECG (donde 0 mV está en el tercio inferior), en EMG el **0 mV está en el centro** (128) porque la señal oscila simétricamente arriba y abajo.

### 12.3 Métricas para Visualización

Además de la señal cruda, el modelo calcula:

| Métrica | Descripción | Unidad |
|---------|-------------|--------|
| **RMS** | Amplitud promedio (root mean square) | mV |
| **MUs Activas** | Cuántas unidades están reclutadas | # |
| **Frecuencia Media** | Promedio de frecuencias de disparo | Hz |
| **% Contracción** | Nivel de excitación como porcentaje | % |

Estas métricas se calculan sobre una ventana de 100 ms y están disponibles para mostrar en la pantalla Nextion.

---

# Parte III: Modelo PPG

---

## 13. PPG: El Pulso de Luz

### 13.1 ¿Qué es el PPG?

El **fotopletismograma (PPG)** es la tecnología que está detrás del oxímetro de pulso que te ponen en el dedo en el hospital. Funciona así:

1. Un **LED** emite luz hacia tu dedo (roja o infrarroja)
2. La luz atraviesa el tejido y la sangre
3. Un **fotodetector** mide cuánta luz llega al otro lado
4. La cantidad de luz absorbida cambia con cada latido del corazón

### 13.2 La Analogía del Globo de Agua

Imagina que miras una linterna a través de un globo lleno de agua coloreada:

- Cuando el globo está **más lleno** → pasa **menos luz** (más absorción)
- Cuando el globo está **menos lleno** → pasa **más luz** (menos absorción)

Tu dedo funciona igual:
- Durante la **sístole** (corazón bombea) → más sangre en el dedo → más absorción
- Durante la **diástole** (corazón se relaja) → menos sangre → menos absorción

### 13.3 La Diferencia con ECG y EMG

| Señal | Tipo | Qué mide | Forma |
|-------|------|----------|-------|
| **ECG** | Eléctrica | Actividad eléctrica del corazón | Ondas PQRST definidas |
| **EMG** | Eléctrica | Actividad de fibras musculares | "Ruido" estructurado |
| **PPG** | Óptica | Volumen de sangre | Pulso suave con muesca |

---

## 14. Anatomía de un Pulso PPG

### 14.1 Las Partes del Pulso

Un pulso PPG típico tiene tres características principales:

```
                Pico Sistólico
                     ▲
                    ╱│╲
                   ╱ │ ╲
                  ╱  │  ╲      Pico Diastólico
                 ╱   │   ╲        ▲
                ╱    │    ╲      ╱│╲
               ╱     │     ╲    ╱ │ ╲
              ╱      │      ╲__╱  │  ╲___  ← Muesca Dicrótica
             ╱       │             │      ╲    (el "valle")
────────────╱────────│─────────────│───────╲────
           0%       15%           40%      100%
                    
                  Fase del Ciclo Cardíaco
```

### 14.2 ¿Qué Representa Cada Parte?

| Parte | Posición | ¿Qué está pasando? |
|-------|----------|-------------------|
| **Pico Sistólico** | 15% del ciclo | El corazón acaba de bombear, llega la onda de presión |
| **Muesca Dicrótica** | 30% del ciclo | La válvula aórtica se cierra (¡clic!) |
| **Pico Diastólico** | 40% del ciclo | La onda de presión "rebota" desde las arterias |

### 14.3 ¿Por qué hay un Segundo Pico?

El pico diastólico es fascinante. Cuando el corazón bombea sangre, crea una onda de presión que viaja por las arterias. Cuando esta onda llega a la **bifurcación aórtica** (donde la aorta se divide en dos para ir a las piernas), parte de la onda **rebota** hacia arriba.

Este eco de la onda de presión es lo que vemos como el pico diastólico. En personas jóvenes con arterias elásticas, este pico es muy visible. En personas mayores con arterias rígidas, el pico se aplana.

---

## 15. Construyendo el Pulso con Gaussianas

### 15.1 El Modelo de Tres Campanas

Construimos el pulso PPG sumando tres "campanas" gaussianas:

```
Campana 1 (Sistólica):     Campana 2 (Muesca):     Campana 3 (Diastólica):
       ███                                                 ██
      █████                       ▼                       ████
     ███████                    ████                     ██████
    █████████                  ██████                   ████████
   ███████████                ████████                 ██████████
──────────────    MENOS    ──────────    MÁS    ──────────────────
   (grande)                  (pequeña)               (mediana)
```

**Suma final:**
```
Sistólica + Diastólica - Muesca = Pulso PPG completo
```

### 15.2 Los Parámetros de Cada Campana

| Campana | Posición | Altura | Ancho |
|---------|----------|--------|-------|
| Sistólica | 15% | 1.0 (referencia) | Estrecha (0.055) |
| Diastólica | 40% | 0.4 (40% de sistólica) | Ancha (0.10) |
| Muesca | 30% | 0.25 (se resta) | Muy estrecha (0.02) |

### 15.3 El Nivel DC: La Base de Todo

A diferencia de ECG y EMG que oscilan alrededor de cero, el PPG tiene un **nivel DC constante**:

```
         ← Componente AC (pulsátil, ~1-5% del total)
        ╭──╮
    ~~~╱    ╲~~~╱╲~~~╱╲~~~
────────────────────────── ← Nivel DC (absorción constante, ~50%)
```

El **Índice de Perfusión (PI)** es simplemente la relación AC/DC:
- PI alto (5-20%) → buena perfusión (sangre fluyendo bien)
- PI bajo (<0.5%) → mala perfusión (shock, frío)

---

## 16. Condiciones Clínicas del PPG

### 16.1 Las 7 Condiciones

| # | Condición | ¿Qué simula? |
|---|-----------|--------------|
| 0 | **NORMAL** | Paciente sano, buena perfusión |
| 1 | **ARRHYTHMIA** | Ritmo irregular (como fibrilación auricular) |
| 2 | **WEAK_PERFUSION** | Shock, hipotensión, manos frías |
| 3 | **STRONG_PERFUSION** | Fiebre, vasodilatación |
| 4 | **VASOCONSTRICTION** | Frío, estrés, vasopresores |
| 5 | **MOTION_ARTIFACT** | Paciente moviéndose |
| 6 | **LOW_SPO2** | Hipoxemia (poco oxígeno) |

### 16.2 Perfusión Débil: El Pulso que Desaparece

En shock o hipoperfusión severa, el pulso PPG casi desaparece:

```
Normal:                    Perfusión Débil:
       ███                        ▄
      █████                      ███
     ███████                    █████
    █████████                  ███████
────────────────           ────────────────
   Amplitud 100%              Amplitud 25%
```

**Cambios:**
- Amplitud reducida al 25%
- Muesca casi invisible
- Taquicardia compensatoria (corazón intenta compensar)

### 16.3 Vasoconstricción: La Muesca Prominente

Cuando las arterias se contraen (frío, estrés), la resistencia vascular aumenta y la muesca se hace muy visible:

```
Normal:                    Vasoconstricción:
       ██                         ██
      ████                       ████
     ██████      vs            ██    ██  ← Muesca muy marcada
    ████████                  ██      ██
────────────────           ────────────────
```

### 16.4 Artefacto de Movimiento: El Ruido

Cuando el paciente mueve la mano, el sensor registra cambios que no son del pulso:

```
Normal:                    Con Movimiento:
     ╭──╮                     ╭──╮
    ╱    ╲                 ▄▂╱█ ▄╲▂▅
   ╱      ╲╱╲             █▃▁ ▂██▂▁█╲▃█
──────────────           ────────────────
   Señal limpia           Señal con ruido
```

---

## 17. De Números a Voltaje Real (PPG)

### 17.1 El Rango del Modelo

El PPG se normaliza a un rango de 0 a 1:

| Valor | Significado |
|-------|-------------|
| 0.0 | Mínima absorción de luz |
| 0.5 | Nivel DC típico |
| 0.65 | Pico sistólico típico |
| 1.0 | Máxima absorción |

### 17.2 Conversión a DAC

El mapeo es directo:

| Valor PPG | Valor DAC | Voltaje Salida |
|-----------|-----------|----------------|
| 0.0 | 0 | 0.00 V |
| 0.5 | 128 | 1.65 V |
| 1.0 | 255 | 3.30 V |

### 17.3 Métricas para Visualización

| Métrica | Descripción | En Pantalla |
|---------|-------------|-------------|
| **HR** | Frecuencia cardíaca | "75 BPM" |
| **PI** | Índice de perfusión | "5.2%" |
| **Estado** | Condición actual | "Normal" |

---

# Parte IV: Diagramas de Flujo

---

## 18. Diagrama de Flujo: Modelo ECG

### 18.1 Flujo Principal

```
┌─────────────────────────────────────────────────────────────────┐
│                     INICIO: ECGModel::getDACValue()             │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿Hay parámetros pendientes Y estamos en inicio de latido?     │
│                    (theta cerca de 0)                           │
└─────────────────────────┬───────────────────────────────────────┘
                          │
              ┌───────────┴───────────┐
              ▼                       ▼
           [SÍ]                     [NO]
              │                       │
              ▼                       │
┌─────────────────────────┐           │
│ Aplicar parámetros      │           │
│ pendientes              │           │
│ Generar nuevo RR        │           │
│ Aplicar variabilidad    │           │
└────────────┬────────────┘           │
              │                       │
              └───────────┬───────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│              INTEGRACIÓN RK4 (4 evaluaciones)                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ k1 = f(t, state)                                         │   │
│  │ k2 = f(t + h/2, state + h/2 * k1)                       │   │
│  │ k3 = f(t + h/2, state + h/2 * k2)                       │   │
│  │ k4 = f(t + h, state + h * k3)                           │   │
│  │ state += h/6 * (k1 + 2*k2 + 2*k3 + k4)                  │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│            CALCULAR CONTRIBUCIONES DE ONDAS                     │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Para cada onda (P, Q, R, S, T):                          │   │
│  │   Δθ = diferencia angular al centro de la onda           │   │
│  │   contribución = a × Δθ × exp(-Δθ²/2b²)                  │   │
│  │   dz/dt -= contribución                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿Condición especial activa?                                    │
└─────────────────────────┬───────────────────────────────────────┘
                          │
    ┌─────────────────────┼─────────────────────┐
    ▼                     ▼                     ▼
[VFib]               [ST Elevation]         [Normal]
    │                     │                     │
    ▼                     ▼                     │
┌────────────┐    ┌────────────────┐            │
│ Generar    │    │ Añadir offset  │            │
│ oscilación │    │ en segmento ST │            │
│ caótica    │    └───────┬────────┘            │
└─────┬──────┘            │                     │
      │                   │                     │
      └───────────────────┴──────────┬──────────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                   CONVERSIÓN A DAC                              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ mV_valor = z (salida del modelo)                         │   │
│  │ normalizado = (mV_valor - (-0.5)) / 2.0                  │   │
│  │ DAC = normalizado × 255                                   │   │
│  │ DAC = constrain(DAC, 0, 255)                             │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      RETORNAR DAC (0-255)                       │
└─────────────────────────────────────────────────────────────────┘
```

### 18.2 Subproceso: Generación de Nuevo Latido

```
┌───────────────────────────────────────────────┐
│         NUEVO LATIDO DETECTADO                │
└──────────────────────┬────────────────────────┘
                       │
                       ▼
┌───────────────────────────────────────────────┐
│  Generar nuevo intervalo RR:                  │
│  RR = 60/HR + gaussianRandom(0, CV×RR)       │
└──────────────────────┬────────────────────────┘
                       │
                       ▼
┌───────────────────────────────────────────────┐
│  Aplicar variabilidad morfológica:            │
│  • Amplitudes: ±5%                            │
│  • Anchos: ±2%                                │
│  • Posiciones: ±0.01 rad                      │
└──────────────────────┬────────────────────────┘
                       │
                       ▼
┌───────────────────────────────────────────────┐
│  ¿Latido ectópico (PVC)?                      │
│  (probabilidad según condición)               │
└──────────────────────┬────────────────────────┘
                       │
           ┌───────────┴───────────┐
           ▼                       ▼
        [SÍ]                     [NO]
           │                       │
           ▼                       │
┌────────────────────┐             │
│ Modificar QRS:     │             │
│ • Sin onda P       │             │
│ • QRS ancho        │             │
│ • T invertida      │             │
└─────────┬──────────┘             │
           │                       │
           └───────────┬───────────┘
                       │
                       ▼
┌───────────────────────────────────────────────┐
│            CONTINUAR GENERACIÓN               │
└───────────────────────────────────────────────┘
```

---

## 19. Diagrama de Flujo: Modelo EMG

### 19.1 Flujo Principal

```
┌─────────────────────────────────────────────────────────────────┐
│                    INICIO: EMGModel::getDACValue()              │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│         DETERMINAR NIVEL DE EXCITACIÓN                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ excitación = params.excitationLevel                       │   │
│  │ SI condición == TREMOR:                                   │   │
│  │     excitación = 0.3 × (0.5 + 0.5×sin(2π×5×t))          │   │
│  │ SI condición == FATIGUE:                                  │   │
│  │     excitación aumenta gradualmente (60%→90%)            │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│         PARA CADA UNIDAD MOTORA (i = 0 a 99):                   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿excitación >= umbral[i]?                                      │
└─────────────────────────┬───────────────────────────────────────┘
                          │
              ┌───────────┴───────────┐
              ▼                       ▼
           [SÍ]                     [NO]
              │                       │
              ▼                       ▼
┌─────────────────────────┐  ┌─────────────────────────┐
│ MU ACTIVA:              │  │ MU INACTIVA:            │
│ • Calcular frecuencia   │  │ • No contribuye         │
│   de disparo (FR)       │  │   (excepto fasciculation)│
│ • Actualizar timer      │  │                         │
└────────────┬────────────┘  └────────────┬────────────┘
              │                           │
              ▼                           │
┌─────────────────────────┐               │
│ ¿Timer >= 1/FR?         │               │
│ (¿Toca disparar?)       │               │
└────────────┬────────────┘               │
              │                           │
    ┌─────────┴─────────┐                 │
    ▼                   ▼                 │
  [SÍ]               [NO]                 │
    │                   │                 │
    ▼                   │                 │
┌────────────────┐      │                 │
│ GENERAR MUAP:  │      │                 │
│ ┌────────────┐ │      │                 │
│ │ Mexican Hat│ │      │                 │
│ │ Wavelet    │ │      │                 │
│ │            │ │      │                 │
│ │ MUAP = A × │ │      │                 │
│ │ (1-t²/σ²)× │ │      │                 │
│ │ exp(-t²/2σ²)│      │                 │
│ └────────────┘ │      │                 │
│ Reset timer    │      │                 │
│ + variabilidad │      │                 │
└───────┬────────┘      │                 │
        │               │                 │
        └───────┬───────┘                 │
                │                         │
                ▼                         │
┌─────────────────────────┐               │
│ Sumar contribución      │               │
│ al buffer de salida     │               │
└────────────┬────────────┘               │
              │                           │
              └─────────────┬─────────────┘
                            │
                            ▼
                    (siguiente MU)
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│              DESPUÉS DE TODAS LAS MUs:                          │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ señal = Σ todas las contribuciones                        │   │
│  │ señal += ruido_gaussiano(0, noiseLevel × 0.1)            │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                   CONVERSIÓN A DAC                              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ DAC = 128 + (señal_mV / 5.0) × 127                       │   │
│  │ DAC = constrain(DAC, 0, 255)                             │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      RETORNAR DAC (0-255)                       │
└─────────────────────────────────────────────────────────────────┘
```

### 19.2 Subproceso: Condiciones Patológicas EMG

```
┌───────────────────────────────────────────────────────────────────┐
│                    MODIFICADORES POR CONDICIÓN                    │
└───────────────────────────────┬───────────────────────────────────┘
                                │
        ┌───────────────────────┼───────────────────────┐
        ▼                       ▼                       ▼
   [MYOPATHY]              [NEUROPATHY]          [FASCICULATION]
        │                       │                       │
        ▼                       ▼                       ▼
┌───────────────┐      ┌───────────────┐      ┌───────────────┐
│ Todas las MUs:│      │ 2/3 de MUs:   │      │ Excitación=0  │
│ amplitud×0.4  │      │ umbral=∞      │      │ Disparos      │
│               │      │ (inactivas)    │      │ espontáneos   │
│ MUAPs pequeños│      │               │      │ aleatorios    │
│               │      │ 1/3 restantes:│      │ (P=0.1%/MU/ms)│
│               │      │ amplitud×2.5  │      │               │
│               │      │ MUAPs gigantes│      │               │
└───────────────┘      └───────────────┘      └───────────────┘
```

---

## 20. Diagrama de Flujo: Modelo PPG

### 20.1 Flujo Principal

```
┌─────────────────────────────────────────────────────────────────┐
│                    INICIO: PPGModel::getDACValue()              │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                   AVANZAR FASE EN EL CICLO                      │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ fase += deltaTime / RR_actual                             │   │
│  │ SI fase >= 1.0:                                           │   │
│  │     fase = fmod(fase, 1.0)                                │   │
│  │     → NUEVO LATIDO DETECTADO                              │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│  ¿Nuevo latido? (fase cruzó de 1.0 a 0)                        │
└─────────────────────────┬───────────────────────────────────────┘
                          │
              ┌───────────┴───────────┐
              ▼                       ▼
           [SÍ]                     [NO]
              │                       │
              ▼                       │
┌─────────────────────────┐           │
│ • Aplicar parámetros    │           │
│   pendientes            │           │
│ • Generar nuevo RR      │           │
│   con variabilidad      │           │
│ • beatCount++           │           │
└────────────┬────────────┘           │
              │                       │
              └───────────┬───────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│              CALCULAR FORMA DEL PULSO                           │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ Pico Sistólico:                                           │   │
│  │   G_sist = A_sist × exp(-(fase-0.15)²/(2×0.055²))        │   │
│  │                                                           │   │
│  │ Muesca Dicrótica:                                         │   │
│  │   G_notch = A_notch × exp(-(fase-0.30)²/(2×0.02²))       │   │
│  │                                                           │   │
│  │ Pico Diastólico:                                          │   │
│  │   G_diast = A_diast × exp(-(fase-0.40)²/(2×0.10²))       │   │
│  │                                                           │   │
│  │ pulso = G_sist + G_diast - G_notch                       │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                CONSTRUIR SEÑAL FINAL                            │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ señal = DC_LEVEL + pulso × AC_SCALE                      │   │
│  │       = 0.5 + pulso × 0.3                                 │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                AÑADIR COMPONENTES ADICIONALES                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ // Baseline wander (respiración ~0.25 Hz)                 │   │
│  │ señal += 0.02 × sin(baselineWander)                      │   │
│  │                                                           │   │
│  │ // Ruido de movimiento (si aplica)                        │   │
│  │ SI motionNoise > 0:                                       │   │
│  │     señal += gaussianRandom(0, motionNoise × 0.1)        │   │
│  │     SI random < 0.2%: señal += spike aleatorio           │   │
│  │                                                           │   │
│  │ // Ruido base                                             │   │
│  │ señal += gaussianRandom(0, noiseLevel × 0.05)            │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                   CONVERSIÓN A DAC                              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ señal = constrain(señal, 0.0, 1.0)                       │   │
│  │ DAC = señal × 255                                         │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────┬───────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────────┐
│                      RETORNAR DAC (0-255)                       │
└─────────────────────────────────────────────────────────────────┘
```

### 20.2 Subproceso: Modificadores por Condición Clínica

```
┌───────────────────────────────────────────────────────────────────┐
│              applyConditionModifiers()                            │
└───────────────────────────────┬───────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│  Reset a valores normales:                                        │
│  systolicAmplitude = 1.0                                          │
│  diastolicAmplitude = 0.4                                         │
│  dicroticDepth = params.dicroticNotch                             │
│  motionNoise = 0                                                  │
└───────────────────────────────┬───────────────────────────────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│                     SWITCH (condición)                            │
└───────────────────────────────┬───────────────────────────────────┘
        │           │           │           │           │
        ▼           ▼           ▼           ▼           ▼
   [NORMAL]  [WEAK_PERF]  [STRONG_PERF] [VASOCONST]  [LOW_SPO2]
        │           │           │           │           │
        ▼           ▼           ▼           ▼           ▼
┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌──────────┐
│(sin      │ │syst=0.25 │ │syst=1.6  │ │syst=0.7  │ │syst=0.5  │
│cambios)  │ │diast=0.08│ │diast=0.7 │ │diast=0.35│ │diast=0.15│
│          │ │notch=0.1 │ │notch=0.35│ │notch=0.45│ │notch=0.15│
│          │ │HR=115    │ │HR=72     │ │          │ │HR=105    │
└──────────┘ └──────────┘ └──────────┘ └──────────┘ └──────────┘
        │           │           │           │           │
        └───────────┴───────────┴───────────┴───────────┘
                                │
                                ▼
┌───────────────────────────────────────────────────────────────────┐
│  Aplicar escala de Perfusion Index:                               │
│  piScale = perfusionIndex / 5.0                                   │
│  piScale = constrain(piScale, 0.1, 2.0)                          │
│  systolicAmplitude *= piScale                                     │
│  diastolicAmplitude *= piScale                                    │
└───────────────────────────────────────────────────────────────────┘
```

### 20.3 Visualización: Las Tres Gaussianas

```
Fase del ciclo:  0%     15%      30%      40%      100%
                 │       │        │        │         │
                 │       ▼        │        │         │
Sistólica:       │      ╭█╮      │        │         │
                 │     ╱███╲     │        │         │
                 │   ╱███████╲   │        │         │
                 │ ╱█████████████╲        │         │
                 └─────────────────────────────────────
                 │               │        │         │
                 │               ▼        │         │
Muesca (-)       │              ╭█╮      │         │
                 │             ╱   ╲     │         │
                 │           ╱       ╲   │         │
                 └─────────────────────────────────────
                 │                       │         │
                 │                       ▼         │
Diastólica:      │                     ╭███╮       │
                 │                   ╱███████╲     │
                 │                 ╱███████████╲   │
                 └─────────────────────────────────────
                 
                 ═══════════════════════════════════
                 
SUMA FINAL:            ╭█╮
(sist+diast-notch)    ╱███╲    ╭█╮
                     ╱█████╲__╱███╲
                   ╱█████████████████╲_______________
                 └─────────────────────────────────────
                        Pico    ↑   Pico
                      Sistólico │ Diastólico
                              Muesca
                            Dicrótica
```

---

## Apéndice: Tablas de Parámetros de Referencia

### A.1 Parámetros Base de las Ondas PQRST (ECG)

| Onda | Posición (grados) | Amplitud | Ancho |
|------|-------------------|----------|-------|
| P | -70° | 1.2 (positiva) | 0.25 (ancha) |
| Q | -15° | -5.0 (negativa) | 0.10 (estrecha) |
| R | 0° (referencia) | 30.0 (la más alta) | 0.10 (estrecha) |
| S | +15° | -7.5 (negativa) | 0.10 (estrecha) |
| T | +100° | 0.75 (positiva) | 0.40 (la más ancha) |

### A.2 Constantes de Variabilidad ECG

| Parámetro | Valor | Efecto |
|-----------|-------|--------|
| Variación de amplitud | ±5% | Simula efecto respiratorio |
| Variación de ancho | ±2% | Pequeñas variaciones de conducción |
| Variación de posición | ±0.6° | Timing sutil entre ondas |
| Variabilidad RR (normal) | 5-8% | Modulación vagal saludable |

### A.3 Rangos por Condición ECG

| Condición | Frecuencia Cardíaca | Variabilidad RR |
|-----------|---------------------|-----------------|
| Normal | 60-100 BPM | 5-8% |
| Taquicardia | 100-180 BPM | 3-6% |
| Bradicardia | 30-59 BPM | 2-5% |
| Fibrilación Auricular | 60-180 BPM | 15-35% |

### A.4 Parámetros de Unidades Motoras (EMG)

| Parámetro | Valor | Fuente |
|-----------|-------|--------|
| Número de MUs | 100 | Modelo Fuglevand |
| Rango de reclutamiento | 0-60% excitación | Fuglevand 1993 |
| Frecuencia mínima | 6 Hz | De Luca 2010 |
| Frecuencia máxima | 50 Hz | De Luca 2010 |
| Ganancia frecuencia | 40 Hz/unidad | De Luca 2010 |
| Variabilidad ISI | ±20% | Literatura |
| Duración MUAP | 12 ms | De Luca 1997 |
| Sigma wavelet | 2 ms | Modelo |

### A.5 Amplitudes de MUAPs por MU

| Tipo de MU | Umbral | Amplitud |
|------------|--------|----------|
| Pequeñas (primeras) | 0-10% | 0.3 mV |
| Medianas | 10-40% | 0.5-1.0 mV |
| Grandes (últimas) | 40-60% | 1.0-1.5 mV |

### A.6 Modificadores por Condición EMG

| Condición | Excitación | Modificación MUs |
|-----------|------------|------------------|
| REST | 0% | Normal |
| MILD | 20% | Normal |
| MODERATE | 50% | Normal |
| STRONG | 80% | Normal |
| MAXIMUM | 100% | Normal |
| TREMOR | 0-60% @ 5Hz | Normal |
| MYOPATHY | 40% | Amplitud × 0.4 |
| NEUROPATHY | 50% | 1/3 MUs, amplitud × 2.5 |
| FASCICULATION | 0% | Disparos aleatorios |
| FATIGUE | 60%→90% | FR × 0.7 progresivo |

### A.7 Parámetros Base PPG (Allen 2007, Elgendi 2012)

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| Posición Sistólica | 15% del ciclo | ~120ms a 75 BPM |
| Posición Muesca | 30% del ciclo | Cierre válvula aórtica |
| Posición Diastólica | 40% del ciclo | Onda reflejada |
| σ Sistólico | 0.055 | Ancho gaussiano |
| σ Diastólico | 0.10 | Más ancha (dispersión) |
| σ Muesca | 0.02 | Muy estrecha |
| Ratio Diast/Sist | 0.40 | Rango: 0.3-0.6 |
| Profundidad Muesca | 0.25 | Rango: 0.1-0.4 |
| Nivel DC | 0.50 | Absorción constante |
| Escala AC | 0.30 | Componente pulsátil |

### A.8 Modificadores por Condición PPG

| Condición | Sistólica | Diastólica | Muesca | HR | PI típico |
|-----------|-----------|------------|--------|-----|-----------|
| NORMAL | 1.0 | 0.4 | 0.25 | — | 2-5% |
| ARRHYTHMIA | 1.0 | 0.4 | 0.25 | — | 2-5% |
| WEAK_PERFUSION | 0.25 | 0.08 | 0.10 | 115 | <0.5% |
| STRONG_PERFUSION | 1.6 | 0.7 | 0.35 | 72 | 10-20% |
| VASOCONSTRICTION | 0.7 | 0.35 | 0.45 | — | 0.5-2% |
| MOTION_ARTIFACT | 1.0 | 0.4 | 0.25 | — | variable |
| LOW_SPO2 | 0.5 | 0.15 | 0.15 | 105 | 1-3% |

### A.9 Conversión DAC por Tipo de Señal

| Señal | Rango Entrada | Centro | Fórmula DAC |
|-------|---------------|--------|-------------|
| ECG | -0.5 a +1.5 mV | ~0.25 mV | (mV+0.5)/2.0 × 255 |
| EMG | -5.0 a +5.0 mV | 0 mV | 128 + (mV/5.0) × 127 |
| PPG | 0.0 a 1.0 | 0.5 | valor × 255 |

---


