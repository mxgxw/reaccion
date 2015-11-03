Red de Acción Comunitaria (Reaccion.net)
========================================

"Reaccion.net es una Red de Acción Comunitaria que hace uso de un
dispositivo electrónico que facilita la comunicación y coordinación
de comunidades en caso de siniestros, canalizando eficazmente la
información de alerta temprana y necesidades básicas durante
y después de una emergencia..."

El dispositivo consiste en un módulo de transmisión de datos
inalámbrico diseñado específicamente para enviar mensajes
pre-acordados en la forma de mensajes cortos tipo "semáforo"
indicando distintos niveles de alerta o impácto en una comunidad.
Los dispositivos almacenan y actualizan la información entre si al
estilo de una red P2P de tal manera que la información se guarda
de manera distribuida en toda la red.

En este proyecto de GitHub encontrarás las notas de diseño,
diagramas eléctricos, diseños de las tabletas de circuito impreso,
hojas técnicas y el sofware necesario para controlar el kit Reaccion.net.

Puedes apoyar este proyecto haciendo un "Fork" del mismo, agregando
mejoras al diseño y/o funcionamiento e incorporándolas de nuevo a
través de un "pull request".

Ayúdanos a materializar esta idea a través de SocialLab iniciado
sesión con Faceebok y haciendo click a "Vota por esta idea":

http://www.socialab.com/ideas/ver/15704

Para más información contactanos a través de nuestro
email: info _arroba_ reaccion *punto* net.

Últimas adiciones
=================

+Agregadas las hojas técnicas de los componentes bajo extra/Datasheets.

Costo/Lista de Materiales
=========================

Encuentra la lista de materiales en el archivo bom.csv

Pre-requisitos/Requerimientos
=============================

Pendiente diseño del módulo de cargador/regulador de voltaje LiPO.

Construcción y uso
==================

// TO DO

Carpetas
========

```
raíz
 |--dsn: Archivos de diseño
 |   |--main: Esquemáticos y diagrama de PCB para Eagle.
 |   |--auxiliar: Esquemático y diagrama de PCB en formato PDF imprimible.
 |--doc: Archivos de documentación para construcción y uso.
 |--src: Código fuente del software de control (Arduino).
 |--extra: Anexos
     |--Datasheets: Hojas técnicas de los componentes utilizados.
     |--Gráficos: Renders y bocetos del diseño.
     |--sketchs: Esquemas a mano rápidamente.
```

Excepción de responsabilidades
==============================

> El presente proyecto se comparte "tal cual" con el único objetivo de que sea útil.
El/los creadores del presente hardware y su software asociado no pueden garantizar su
correcto funcionamiento bajo ninguna circunstancia. El/Los autor/es de este proyecto
no podrá/n hacerse responsable/s de cualquier pérdida de carácter material, personal o
económico a su persona o terceros derivados de la utilización del mismo. Este diseño
utiliza componentes emisores de radiofrecuencia que podrían requerir algún tipo de
licenciamiento o pruebas de emisiones antes de su puesta en funcionamiento, asegurate
de estar al tanto de las regulaciones locales respecto al tema.

Licenciamiento
==============
El código fuente del software utilizado en este proyecto está protegido bajo la
licencia GNU/GPL v2.0.

Los archivos de diseño principales y auxiliares están liberados bajo la licencia
CC-BY-SA 3.0 ( https://creativecommons.org/licenses/by-sa/3.0/ ).

> La plantilla de este README.md ha sido desarrollada por la comunidad openhardware.sv
con el objetivo de facilitar la documentación de proyectos. Esta plantilla está protegida
bajo la licencia CC BY, puedes modificarla y redistribuirla manteniendo esta nota de
atribución del autor.
