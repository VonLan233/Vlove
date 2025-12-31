# 4. Future Work

## 4.1 Machine Learning-Based Gesture Recognition

The current constraint-based gesture recognition system, while deterministic and interpretable, has inherent limitations in handling subtle gesture variations and user-specific hand characteristics. We plan to develop a **machine learning pipeline** that leverages the glove's sensor data for more robust and adaptive recognition.

**Research Value**: By collecting and open-sourcing a labeled gesture dataset from multiple users, we can contribute to the broader research community working on hand gesture recognition. Unlike camera-based datasets (e.g., SHREC, DHG), our potentiometer-based approach provides direct joint angle measurements unaffected by lighting conditions or occlusion. This complementary modality could enable multi-modal fusion research combining visual and proprioceptive data.

**Technical Direction**: We envision a hybrid system where the existing rule-based matcher handles well-defined gestures with high confidence, while a lightweight neural network (e.g., 1D CNN or LSTM) classifies ambiguous cases. This approach preserves the low-latency benefits of rule-based matching while gaining the flexibility of learned representations. Additionally, we plan to implement **user-specific calibration through transfer learning**, allowing the model to adapt to individual hand geometries with minimal additional training data.

## 4.2 Embodied Intelligence and Robotic Hand Training

The rise of embodied AI has created significant demand for high-quality human hand manipulation data. Commercial motion capture systems capable of capturing dexterous hand movements cost tens of thousands of dollars, creating a barrier for researchers and hobbyists. Our low-cost glove presents an opportunity to **democratize data collection for robotic hand training**.

**Research Value**: Imitation learning and teleoperation are key paradigms in robot manipulation research. By providing an affordable device that captures finger articulation in real-time, we enable researchers to collect demonstration data for tasks like grasping, tool use, and object manipulation. The captured trajectories can be used for behavior cloning, inverse reinforcement learning, or as guidance for reinforcement learning policies.

**Technical Direction**: We plan to develop a **ROS (Robot Operating System) integration package** that publishes finger joint angles as standard sensor messages. This would allow direct teleoperation of simulated or physical robotic hands. Furthermore, we aim to create a standardized data format for storing manipulation demonstrations, including synchronized finger positions, timestamps, and optional task annotations. Collaboration with the robotics community could establish Vlove as a reference platform for low-cost dexterous data collection.

## 4.3 Enhanced Human-Computer Interaction

Beyond VR gaming and gesture recognition, the glove platform opens possibilities for novel interaction paradigms. We aim to explore **multi-modal and context-aware interaction** that combines hand gestures with other input modalities.

**Research Value**: Traditional input devices (mouse, keyboard, touchscreen) constrain interaction to 2D surfaces. Hand gesture interfaces enable spatial, 3D interaction that is more intuitive for certain tasks—3D modeling, musical performance, surgical simulation, and accessibility applications for users with motor impairments. Our open-source platform provides a testbed for HCI researchers to prototype and evaluate new interaction techniques.

**Technical Direction**: We plan to integrate the glove with additional sensing modalities:

1. **IMU-based Hand Tracking**: Combining finger articulation with wrist orientation enables full 6-DOF hand pose estimation, useful for VR controllers and sign language recognition.

2. **Haptic Feedback Integration**: Adding vibration motors or servo-based force feedback would enable bidirectional interaction, allowing users to "feel" virtual objects.

3. **Voice + Gesture Fusion**: Combining gesture recognition with speech commands could create more natural interfaces—pointing at an object while saying "delete this" or making a pinch gesture while saying "smaller."

4. **Accessibility Applications**: Developing gesture-to-text or gesture-to-speech systems for users who cannot use traditional input devices, potentially enabling new communication methods for individuals with disabilities.

## 4.4 Summary of Future Directions

| Direction | Primary Value | Target Community |
|-----------|---------------|------------------|
| ML Gesture Recognition | Open dataset, adaptive recognition | ML researchers, gesture recognition community |
| Embodied Intelligence | Low-cost manipulation data collection | Robotics researchers, imitation learning |
| Enhanced Interaction | Novel input modalities, accessibility | HCI researchers, VR/AR developers, accessibility advocates |

These directions are not mutually exclusive—advances in one area can enable progress in others. For example, improved gesture recognition through machine learning directly benefits both robotic teleoperation (more precise control) and human-computer interaction (more expressive input vocabulary). We welcome collaboration from researchers and developers interested in any of these directions.
