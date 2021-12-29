package threads.thor;

import java.util.Objects;

public class MethodStat {
    Integer id;
    long startCount;
    long endCount;
    long time;

    public MethodStat(Integer id, long startCount, long endCount) {
        this.id = id;
        this.startCount = startCount;
        this.endCount = endCount;
    }

    public Integer getId() {
        return id;
    }

    public void setId(Integer id) {
        this.id = id;
    }

    public long getStartCount() {
        return startCount;
    }

    public void setStartCount(long startCount) {
        this.startCount = startCount;
    }

    public long getEndCount() {
        return endCount;
    }

    public void setEndCount(long endCount) {
        this.endCount = endCount;
    }

    @Override
    public String toString() {
        return "MethodStat{" +
                "id=" + id +
                ", startCount=" + startCount +
                ", endCount=" + endCount +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) return true;
        MethodStat that = (MethodStat) o;
        return Objects.equals(id, that.id) &&
                Objects.equals(startCount, that.startCount) &&
                Objects.equals(endCount, that.endCount);
    }

    @Override
    public int hashCode() {
        return Objects.hash(id, startCount, endCount);
    }
}
