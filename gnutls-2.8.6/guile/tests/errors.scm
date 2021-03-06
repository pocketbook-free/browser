;;; GNUTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007  Free Software Foundation
;;;
;;; GNUTLS is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU Lesser General Public
;;; License as published by the Free Software Foundation; either
;;; version 2.1 of the License, or (at your option) any later version.
;;;
;;; GNUTLS is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Lesser General Public License for more details.
;;;
;;; You should have received a copy of the GNU Lesser General Public
;;; License along with GNUTLS; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

;;; Written by Ludovic Court�s <ludo@chbouib.org>.


;;;
;;; Test the error/exception mechanism.
;;;

(use-modules (gnutls))

(dynamic-wind
    (lambda ()
      #t)

    (lambda ()
      (let ((s (make-session connection-end/server)))
        (catch 'gnutls-error
          (lambda ()
            (handshake s))
          (lambda (key err function . currently-unused)
            (exit (and (eq? key 'gnutls-error)
                       err
                       (string? (error->string err))
                       (eq? function 'handshake)))))))

    (lambda ()
      ;; failure
      (exit 1)))

;;; arch-tag: 73ed6229-378d-4a12-a5c6-4c2586c6e3a2
