<?xml version="1.0"?>
<VirtualBox xmlns="http://www.innotek.de/VirtualBox-settings" version="1.9-linux">
  <Machine uuid="{52e19036-661c-4fbc-b4ce-56648c257230}" name="Escape v0.1" OSType="Other"
           snapshotFolder="Snapshots" lastStateChange="2012-12-23T11:19:54Z">
    <Hardware version="2">
      <CPU count="4" hotplug="false">
        <HardwareVirtEx enabled="true" exclusive="true"/>
        <HardwareVirtExNestedPaging enabled="true"/>
        <HardwareVirtExVPID enabled="true"/>
        <PAE enabled="false"/>
        <HardwareVirtExLargePages enabled="false"/>
        <HardwareVirtForce enabled="false"/>
      </CPU>
      <Memory RAMSize="64"/>
      <UART>
        <Port slot="0" enabled="false" IOBase="0x3f8" IRQ="4" hostMode="Disconnected"/>
      </UART>
      <BIOS>
        <ACPI enabled="true"/>
        <IOAPIC enabled="true"/>
      </BIOS>
      <Network>
        <Adapter slot="0" enabled="true" MACAddress="0800276A040A" cable="true" type="82540EM">
          <NAT/>
        </Adapter>
      </Network>
    </Hardware>
  </Machine>
</VirtualBox>
