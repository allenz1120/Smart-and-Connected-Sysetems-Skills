#  Skill 29

Author: Allen Zou

Date: 2020-11-13
-----

## Summary
-- Identify weaknesses in your overall system with respect to compromise. --

One weakness I recognize is that our Node.js system that we use to send and receive data to a server has a huge lack of privacy. Anyone could potentially access personal data about our car and see exactly where we are and what actions we are taking (i.e they can see all of our data). By simply navigating to our server they can see all of our data. Additionally, another potential weakness is that if someone were to steal our system, the files containing all of our data would be accessible. Finally, if the server we are using develops some sort of problem (slow page loads, hardware/software issue, physical hardware configuration issues, and even cybersecurity attacks) then all users could be affected. 

-- What ways can a bad guy attack your specific system (List 5) --

- They could DDOS the router that we are receiving or sending information from and thus stop our control of the car. 
- A person could pretend to be our router and we could connect to their domain and they would have access to all our car’s information and controls. 
- Our server location can be tracked and accessed by simply knowing the IP our system is connected to, thus allowing them to take control of our car. 
- Physical security, someone can unplug/disconnect our connection to the router which causes us to lose all connection with the car. 
- Susceptible to man-in-the-middle attacks where someone can sniff our packets and 

-- Describe ways that you can overcome these attacks. In what ways can you mitigate these issues? --

We can overcome these certain attacks by implementing security measures from other skills. For example, we can assign a unique ID to certain sensors/devices on the car. This will allow only people who know the ID to access the information from these sensors. Additionally, we could also add a unique username and password to the server which would prevent others from viewing our information. 


## Issues that Little has provided:
An adversary can take control of an embedded device (such as an ESP32)
Falsifying sensor data
Falsifying control and actuation
Combat this by having authentication before any data can be accessed or changed
You can use google authentication and the firebase api

An adversary replaces embedded device with own device, and falsifies data and control

An adversary listens in or modifies data between embedded device and server (“man-in-the-middle” or “eavesdrop” attack)
Encrypt messages and packets when sending over the internet to prevent man-in-the-middle attacks
HTTPS

An adversary takes control of server (local or remote server)
And is able to look at all data from all sensors
And is able to control and actuate all devices

An adversary interrupts flow of information (denial of service – DOS)
Set a watchdog to detect for a repeated number of connection requests and if that number of requests exceeds a certain number within a certain timeframe, we can kick that IP
Set timeouts for open connections that are not doing anything

An adversary physically attacks embedded device or server – steals one
Keep the number of locations for embedded systems low
Remove all unnecessary connections to limit the number of “doors” that hackers can access
And what is the nature of the damage? There will be a range of outcomes
Gain data that provides an advantage (e.g., faster GPS routing of car)
Learn about when someone is home or not (e.g., robbers); loss of privacy
Possibly life-critical: overrides safety (causes a fire, causes collision, etc.)
Economic loss (turn off irrigation, cause crops to fail)



## Sketches and Photos
Sketch of overall flow for driving the car:
![IMG-8397](https://user-images.githubusercontent.com/50682462/99042380-a1657f80-255a-11eb-8a42-3ffc09bc646b.jpg)

## Modules, Tools, Source Used Including Attribution
Citations:
Sheets, David. "Architecting Cybersecurity Into Embedded Systems." Signal 73.5 (2019): 38-40. Web.
Lee, Ruby B. "Rethinking Computers for Cybersecurity." Computer 48.4 (2015): 16-25. Web.


## Supporting Artifacts
N/A

-----
