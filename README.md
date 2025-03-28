# STUSB4500 – Driver C++ moderne pour ESP-IDF

`STUSB4500` est une bibliothèque C++ moderne permettant de configurer et piloter simplement le circuit STUSB4500 via I2C avec ESP-IDF.  
Elle prend en charge la lecture/écriture des PDOs, la gestion de la mémoire NVM, et propose un mécanisme de synchronisation automatique basé sur la broche `ALERT`.

---

## 📦 Fonctions principales

- Abstraction haut niveau pour la lecture/écriture des PDOs
- Modification et persistance de la configuration dans la mémoire NVM
- Lecture automatique des données lors du démarrage
- Mise à jour automatique via la broche `ALERT` ou périodique
- Intégration propre avec la classe `I2CDevice`

---

## 🔧 Installation

Placer les fichiers de la bibliothèque dans un dossier `components/stusb4500/` de votre projet ESP-IDF.

Ajoutez dans votre `CMakeLists.txt` principal :

```cmake
idf_component_register(
    SRCS 
    .....
    REQUIRES STUSB4500
) 
```

---

## ✨ Utilisation

### Création du périphérique

```cpp
#include "stusb4500.hpp"
using namespace stusb4500;

auto i2c = std::make_shared<I2CDevice>(
    I2C_NUM_0, GPIO_NUM_21, GPIO_NUM_22, 0x28, 400000
);

STUSB4500 stusb(i2c);
```

---

### Lecture des PDOs

```cpp
float v = stusb.get_voltage(1);
float i = stusb.get_current(1);
```

---

### Écriture dans la configuration

```cpp
stusb.set_voltage(2, 9.0f);
stusb.set_current(2, 2.0f);
stusb.set_voltage(3, 15.0f);
```

---

### Sauvegarde en mémoire NVM

```cpp
stusb.write_sectors(); // Persiste la config actuelle dans le STUSB4500
```

---

### Détection automatique via broche ALERT

```cpp
stusb.configure_alert_pin(GPIO_NUM_25); // Active la gestion auto via interruption
```

La bibliothèque surveille :
- la broche `ALERT` (si configurée),
- la présence du périphérique,
- la cohérence de la configuration avec un profil par défaut.

---

### Écriture automatique de la configuration par défaut

Vous pouvez modifier la configuration NVM compilée dans :

```cpp
// stusb4500_conf.hpp
static constexpr uint8_t default_sector_config[5][8] = {
    {0x00, 0x00, 0xB0, 0xAA, 0x00, 0x45, 0x00, 0x00},
    ...
};
```

---

## 📄 Licence

Ce projet est distribué sous la licence **Apache License 2.0**.  
Vous pouvez l’utiliser librement dans des projets personnels ou commerciaux.

Voir le fichier [`LICENSE`](./LICENSE) pour plus d’informations.

---

## 👨‍💻 Auteur

Développé par **Lionel Orcil** pour **IOBEWI**.
