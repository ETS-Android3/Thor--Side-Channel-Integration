package threads.thor;

public class SideChannelValue {

    Long systemTime;
    Long timeStamp;
    String address;
    Long count;
    Integer scanTime;

    /**
     * Constructor for this POJO class to store side channel values
     */
    public SideChannelValue() {
    }

    public Long getSystemTime() {
        return systemTime;
    }

    public void setSystemTime(Long systemTime) {
        this.systemTime = systemTime;
    }

    public Long getTimeStamp() {
        return timeStamp;
    }

    public void setTimeStamp(Long timeStamp) {
        this.timeStamp = timeStamp;
    }

    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        this.address = address;
    }

    public Long getCount() {
        return count;
    }

    public void setCount(Long count) {
        this.count = count;
    }

    public Integer getScanTime() {
        return scanTime;
    }

    public void setScanTime(Integer scanTime) {
        this.scanTime = scanTime;
    }
}