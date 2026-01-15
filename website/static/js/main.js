document.addEventListener('DOMContentLoaded', function() {
    document.querySelectorAll('a[href^="#"]').forEach(function(anchor) {
        anchor.addEventListener('click', function(e) {
            e.preventDefault();
            var target = document.querySelector(this.getAttribute('href'));
            if (target) {
                target.scrollIntoView({
                    behavior: 'smooth',
                    block: 'start'
                });
            }
        });
    });

    var features = document.querySelectorAll('.feature, .download-card, .step, .req-card');
    
    var observer = new IntersectionObserver(function(entries) {
        entries.forEach(function(entry) {
            if (entry.isIntersecting) {
                entry.target.style.opacity = '1';
                entry.target.style.transform = 'translateY(0)';
            }
        });
    }, {
        threshold: 0.1,
        rootMargin: '0px 0px -50px 0px'
    });

    features.forEach(function(el) {
        el.style.opacity = '0';
        el.style.transform = 'translateY(20px)';
        el.style.transition = 'opacity 0.5s, transform 0.5s';
        observer.observe(el);
    });

    document.querySelectorAll('pre code').forEach(function(block) {
        var wrapper = document.createElement('div');
        wrapper.style.position = 'relative';
        block.parentNode.parentNode.insertBefore(wrapper, block.parentNode);
        wrapper.appendChild(block.parentNode);

        var copyBtn = document.createElement('button');
        copyBtn.textContent = 'Copy';
        copyBtn.style.cssText = 'position:absolute;top:8px;right:8px;padding:4px 12px;' +
            'background:#2a2a3a;color:#8888a0;border:none;border-radius:4px;' +
            'cursor:pointer;font-size:12px;font-family:inherit;';
        
        copyBtn.addEventListener('click', function() {
            navigator.clipboard.writeText(block.textContent).then(function() {
                copyBtn.textContent = 'Copied';
                setTimeout(function() {
                    copyBtn.textContent = 'Copy';
                }, 2000);
            });
        });

        wrapper.appendChild(copyBtn);
    });
});

