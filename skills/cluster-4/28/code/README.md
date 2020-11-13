# Code Readme

This program was created by combining the udp_client and udp_server code examples and our skill 25 code. 

# State Diagram
The program revolves around three states: ELECTION_STATE, LEADER_STATE, FOLLOWER_STATE. All FOBs enter the election in the ELECTION_STATE and it represents FOBs that are still in contension for becoming the Leader. A FOB is moved to the FOLLOWER_STATE if it realizes that it's ID is higher than the FOBs that is is communicating to which means that it is now out of the race. Lastly, once the election times out, a FOB moves from election to the LEADER_STATE which signifies that it is the declared leader.

# Timers and Timeouts
To keep all the FOBs synced up, we implemented a timer task that decrements timeout variables. We have two timeout variables and 4 timeout constants. The two timeout variables are: a general timeout variable and a udp timeout. The general timeout varibale is dependent on each state and will send a FOB back to election state when it hits zero. The udp timeout is also dependent on the state and tells the FOB to send a UDP message to all the other FOBs when it hits zero. The 4 timeout constants are used to reset the timeout to a specific value and they are the ELECTION_TIMEOUT (15 sec), LEADER_TIMEOUT (30 sec) HEARTBEAT (1 sec) UDP_TIMER (3 sec). 

# Election State
In the ELECTION_STATE, the timeout is initally set to ELECTION_TIMEOUT and the udp timeout is initally set to UDP_TIMER. This allows the FOB to sends UDP messages to all other FOBs every 3 seconds. Once a FOB receives a message from another FOB, it compares the received ID with the FOB's personal ID. If the personal ID is smaller than the received ID, it resets the timeout back to ELECTION_TIMEOUT and continues to wait for messages. If the received ID is larger, the FOB will move states to the FOLLOWER_STATE and stop transmitting messages. Once there is only one FOB left, the timeout will go to zero which sends the FOB to the LEADER_STATE.

# Follower State
In the FOLLOWER_STATE, the FOB knows that it is out of the election race. It stops sending udp client messages and just listens for heartbeats from a leader. The timeout is initally set to LEADER_TIMEOUT which is 30 seconds and if it receives a udp message that has the status leaderHeartbeat with the content of "Alive", it will reset the timeout to 30 seconds and continue listening. If a leader is broken / removed and stops sending udp heartbeat messages, the timeout varibale will reach zero and return to the ELECTION_STATE. Another scenario where the FOB can go back to ELECTION_STATE is if a new device is connected and its
