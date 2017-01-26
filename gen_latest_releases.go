package main;

import (
    "fmt"
    "os"
    "strings"
    "net/http"
    "io/ioutil"
    "bufio"
)

func Exit(value int)  {
    os.Exit(value)
}

func Check(err error) {
    if err != nil {
        fmt.Printf("Error: %s\n", err)
        Exit(1)
    }
}

func UrlDataGet(url string) string {
    req, err := http.Get(url)
    Check(err)
    defer req.Body.Close()

    if req.StatusCode != http.StatusOK {
        return "";
    }

    bytes, err := ioutil.ReadAll(req.Body)
    Check(err)

    return string(bytes)
}


func GetDebian(arch string) string {
    base_url := "http://cdimage.debian.org/debian-cd/current/multi-arch/iso-cd";

    data := UrlDataGet(base_url)

    tag_start := "<a href=\"";
    tag_full := tag_start + "debian-"
    idx := strings.Index(data, tag_full)

    str := data[idx + len(tag_start) :len(data)];
    end := strings.Index(str, "\"") 
    filename := str[0:end]

    return fmt.Sprintf("%s/%s", base_url, filename)
}

func newestURL(data string, major_start int, major_end int) string {
    var found string = "";
    for major := major_start; major < major_end; major++ {
        for minor := 0; minor < 10; minor++ {
            tmp := fmt.Sprintf("%d.%d", major, minor)
            if strings.Contains(data, tmp) {
                found = tmp;
            }
        }
    }
    return found;
}

func GetFreeBSD(arch string) string {

    const FREEBSD_MAJOR_START = 11
    const FREEBSD_MAJOR_END = 20
    base_url := "http://ftp.freebsd.org/pub/FreeBSD/releases/ISO-IMAGES";

    data := UrlDataGet(base_url)

    var version string = newestURL(data, FREEBSD_MAJOR_START, FREEBSD_MAJOR_END)

    if version != "" {
        return fmt.Sprintf("%s/%s/FreeBSD-%s-RELEASE-%s-memstick.img", base_url, version, version, arch)
    }

    return version;
}

func GetOpenBSD(arch string) string {
    const OPENBSD_MAJOR_START = 6;
    const OPENBSD_MAJOR_END = 10;
    base_url := "http://mirror.ox.ac.uk/pub/OpenBSD/";
 
    data := UrlDataGet(base_url) 

    var version string  = newestURL(data, OPENBSD_MAJOR_START, OPENBSD_MAJOR_END)

    // hack for changing 6.0 to 60 for filename on server
    img_ver := strings.Replace(version, ".", "", -1)
    
    if version != "" {
        return (fmt.Sprintf("%s%s/%s/install%s.fs", base_url, version, arch, img_ver))
    }

    return ("");
}

func LatestURL(os string, arch string) string {
    var url string = "";

    switch (os) {
        case "OpenBSD":
        url = GetOpenBSD(arch)
        break;

        case "FreeBSD":
        url = GetFreeBSD(arch)
        break;

        case "Debian":
        url = GetDebian(arch)
        break;
    }

    if len(url) == 0 {
        fmt.Printf("Error: LatestURL %s %s\n", os, arch)
        Exit(1)
    }

    return (url)
}

func distroGetAll() map[string]string {

    distros := make(map[string]string)

    distros["OpenBSD (amd64)"] = LatestURL("OpenBSD", "amd64")
    distros["OpenBSD (i386)"]  = LatestURL("OpenBSD", "i386")  
    distros["FreeBSD (i386)"]  = LatestURL("FreeBSD", "i386")
    distros["FreeBSD (amd64)"] = LatestURL("FreeBSD", "amd64")  
    distros["Debian GNU/Linux (i386/amd64)"] = LatestURL("Debian", "amd64")

    return distros;
}

func main() {

    if len(os.Args) != 2 {
        fmt.Println("You must provide a file to write output to!")
        Exit(1)
    }

    file := os.Args[1];

    distros := distroGetAll()

    f, err := os.Create(file)
    Check(err)
    defer f.Close()

    w := bufio.NewWriter(f)
    
    for name, url := range distros {
        fmt.Fprintf(w, "%s=%s\n", name, url)
        fmt.Printf("Adding: %s with URL: %s\n", name, url)
    }

    fmt.Printf("Saved to %s", file)
    w.Flush()

    Exit(0)
}